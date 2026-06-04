/*
 * ============================================================================
 *  PEARL PIPE — DLL Tarafı Named Pipe Gonderici
 * ============================================================================
 *  Bu dosya Pearl Guard DLL'ine dahil edilir.
 *  PearlMonitor.exe ile Named Pipe uzerinden iletisim kurar.
 *
 *  Kullanim:
 *    #include "PearlPipe.h"
 *
 *    PearlPipe::Init();                          // DLL Init'te cagir
 *    PearlPipe::SendPacketLog(true, 0x31, data, len);  // Paket logu
 *    PearlPipe::SendHPChange(oldHP, newHP, maxHP);      // HP degisimi
 *    PearlPipe::SendSkillUse(skillID, target, src, "Fire Ball");
 *    PearlPipe::Shutdown();                      // DLL kapanista cagir
 *
 *  Monitor yoksa: hicbir sey yapmaz, SIFIR maliyet.
 *  Monitor varsa: async yazma, oyunu BLOKLAMAZ.
 *
 *  (c) 2026 MalaysiaKO Community
 * ============================================================================
 */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <mutex>
#include <atomic>
#include <string>
#include <deque>
#include <thread>

// Mesaj tipleri — PearlMonitor.cpp ile AYNI olmali
namespace PipeMsg {
    enum Type : uint8_t {
        PACKET_SEND  = 0x01,
        PACKET_RECV  = 0x02,
        HP_CHANGE    = 0x10,
        MP_CHANGE    = 0x11,
        EXP_CHANGE   = 0x12,
        GOLD_CHANGE  = 0x13,
        LEVEL_CHANGE = 0x14,
        SP_CHANGE    = 0x15,
        ITEM_USE     = 0x20,
        ITEM_GET     = 0x21,
        ITEM_DROP    = 0x22,
        ITEM_TRADE   = 0x23,
        ITEM_UPGRADE = 0x24,
        ITEM_MOVE    = 0x25,
        SKILL_USE    = 0x30,
        SKILL_RECV   = 0x31,
        ATTACK       = 0x40,
        DEAD         = 0x41,
        REGEN        = 0x42,
        SECURITY     = 0x50,
        HOOK         = 0x51,
        ERR          = 0x60,
        WARNING      = 0x61,
        ZONE_CHANGE  = 0x70,
        CHAT         = 0x80,
        MERCHANT     = 0x90,
        PARTY        = 0xA0,
        CLAN         = 0xA1,
        WAREHOUSE    = 0xB0,
        PUS          = 0xB1,
        QUEST        = 0xC0,
        STATUS       = 0xF0,
        HEARTBEAT    = 0xFE,
        HANDSHAKE    = 0xFF
    };
}

class PearlPipe {
private:
    static HANDLE            s_hPipe;
    static std::atomic<bool> s_connected;
    static std::atomic<bool> s_running;
    static std::mutex        s_writeMutex;
    static std::thread       s_reconnThread;
    static DWORD             s_lastReconnect;

    // ── Ham pipe yazma ───────────────────────────────────────────────────
    static bool RawWrite(const uint8_t* data, size_t len)
    {
        if (!s_connected.load() || s_hPipe == INVALID_HANDLE_VALUE)
            return false;

        std::lock_guard<std::mutex> lock(s_writeMutex);
        DWORD written = 0;
        BOOL ok = WriteFile(s_hPipe, data, (DWORD)len, &written, NULL);
        if (!ok || written != len)
        {
            // Baglanti koptu
            s_connected = false;
            CloseHandle(s_hPipe);
            s_hPipe = INVALID_HANDLE_VALUE;
            return false;
        }
        return true;
    }

    // ── Mesaj olustur ve gonder ──────────────────────────────────────────
    static bool SendMsg(PipeMsg::Type type, const uint8_t* payload, uint16_t payloadLen)
    {
        if (!s_connected.load()) return false;

        uint8_t header[3];
        header[0] = (uint8_t)type;
        header[1] = (uint8_t)(payloadLen & 0xFF);
        header[2] = (uint8_t)((payloadLen >> 8) & 0xFF);

        // Header + payload tek seferde gonder
        uint8_t buf[65536];
        if (3 + payloadLen > sizeof(buf)) return false;
        memcpy(buf, header, 3);
        if (payloadLen > 0 && payload)
            memcpy(buf + 3, payload, payloadLen);

        return RawWrite(buf, 3 + payloadLen);
    }

    // ── Yeniden baglanti thread'i ────────────────────────────────────────
    static void ReconnectThread()
    {
        while (s_running.load())
        {
            if (!s_connected.load())
            {
                DWORD now = GetTickCount();
                if (now - s_lastReconnect < 2000)
                {
                    Sleep(500);
                    continue;
                }
                s_lastReconnect = now;

                HANDLE h = CreateFileA(
                    "\\\\.\\pipe\\PearlMonitor",
                    GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                );

                if (h != INVALID_HANDLE_VALUE)
                {
                    DWORD mode = PIPE_READMODE_BYTE;
                    SetNamedPipeHandleState(h, &mode, NULL, NULL);

                    s_hPipe = h;
                    s_connected = true;

                    // Handshake gonder
                    const char* ver = "v1.0";
                    SendMsg(PipeMsg::HANDSHAKE, (const uint8_t*)ver, (uint16_t)(strlen(ver) + 1));
                }
            }
            Sleep(1000);
        }
    }

    // ── Yardimci: uint16 LE yaz ──────────────────────────────────────────
    static void WriteU16(uint8_t* p, uint16_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
    static void WriteU32(uint8_t* p, uint32_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF; }
    static void WriteI32(uint8_t* p, int32_t v)  { WriteU32(p, (uint32_t)v); }

public:
    // ====================================================================
    //  YASAM DONGUSU
    // ====================================================================

    static void Init()
    {
        if (s_running.load()) return;
        s_running = true;
        s_reconnThread = std::thread(ReconnectThread);
    }

    static void Shutdown()
    {
        s_running = false;
        s_connected = false;
        if (s_hPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(s_hPipe);
            s_hPipe = INVALID_HANDLE_VALUE;
        }
        if (s_reconnThread.joinable())
            s_reconnThread.detach();
    }

    static bool IsConnected() { return s_connected.load(); }

    // ====================================================================
    //  PAKET LOGLARI
    // ====================================================================

    // isSend: true=giden, false=gelen
    static void SendPacketLog(bool isSend, uint8_t opcode, const uint8_t* rawData, uint16_t rawSize)
    {
        if (!s_connected.load()) return;

        uint8_t buf[512];
        buf[0] = opcode;
        WriteU16(&buf[1], rawSize);
        uint16_t copyLen = (rawSize > 500) ? 500 : rawSize;
        if (copyLen > 0 && rawData)
            memcpy(&buf[3], rawData, copyLen);

        SendMsg(isSend ? PipeMsg::PACKET_SEND : PipeMsg::PACKET_RECV, buf, 3 + copyLen);
    }

    // ====================================================================
    //  STAT DEGISIMLERI
    // ====================================================================

    static void SendHPChange(int32_t oldHP, int32_t newHP, int32_t maxHP)
    {
        uint8_t buf[12];
        WriteI32(&buf[0], oldHP);
        WriteI32(&buf[4], newHP);
        WriteI32(&buf[8], maxHP);
        SendMsg(PipeMsg::HP_CHANGE, buf, 12);
    }

    static void SendMPChange(int32_t oldMP, int32_t newMP, int32_t maxMP)
    {
        uint8_t buf[12];
        WriteI32(&buf[0], oldMP);
        WriteI32(&buf[4], newMP);
        WriteI32(&buf[8], maxMP);
        SendMsg(PipeMsg::MP_CHANGE, buf, 12);
    }

    static void SendEXPChange(int64_t oldEXP, int64_t newEXP)
    {
        uint8_t buf[16];
        WriteU32(&buf[0], (uint32_t)(oldEXP & 0xFFFFFFFF));
        WriteU32(&buf[4], (uint32_t)((oldEXP >> 32) & 0xFFFFFFFF));
        WriteU32(&buf[8], (uint32_t)(newEXP & 0xFFFFFFFF));
        WriteU32(&buf[12], (uint32_t)((newEXP >> 32) & 0xFFFFFFFF));
        SendMsg(PipeMsg::EXP_CHANGE, buf, 16);
    }

    static void SendGoldChange(uint32_t oldGold, uint32_t newGold)
    {
        uint8_t buf[8];
        WriteU32(&buf[0], oldGold);
        WriteU32(&buf[4], newGold);
        SendMsg(PipeMsg::GOLD_CHANGE, buf, 8);
    }

    static void SendLevelChange(uint16_t oldLvl, uint16_t newLvl)
    {
        uint8_t buf[4];
        WriteU16(&buf[0], oldLvl);
        WriteU16(&buf[2], newLvl);
        SendMsg(PipeMsg::LEVEL_CHANGE, buf, 4);
    }

    static void SendSPChange(uint32_t oldSP, uint32_t newSP)
    {
        uint8_t buf[8];
        WriteU32(&buf[0], oldSP);
        WriteU32(&buf[4], newSP);
        SendMsg(PipeMsg::SP_CHANGE, buf, 8);
    }

    // ====================================================================
    //  ITEM ISLEMLERI
    // ====================================================================

    static void SendItemUse(uint32_t itemID, uint8_t slot, uint16_t count, const char* itemName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], itemID);
        buf[4] = slot;
        WriteU16(&buf[5], count);
        uint16_t nameLen = (uint16_t)(strlen(itemName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[7], itemName, nameLen);
        SendMsg(PipeMsg::ITEM_USE, buf, 7 + nameLen);
    }

    static void SendItemGet(uint32_t itemID, uint8_t slot, uint16_t count, const char* itemName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], itemID);
        buf[4] = slot;
        WriteU16(&buf[5], count);
        uint16_t nameLen = (uint16_t)(strlen(itemName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[7], itemName, nameLen);
        SendMsg(PipeMsg::ITEM_GET, buf, 7 + nameLen);
    }

    static void SendItemDrop(uint32_t itemID, uint8_t slot, uint16_t count, const char* itemName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], itemID);
        buf[4] = slot;
        WriteU16(&buf[5], count);
        uint16_t nameLen = (uint16_t)(strlen(itemName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[7], itemName, nameLen);
        SendMsg(PipeMsg::ITEM_DROP, buf, 7 + nameLen);
    }

    static void SendItemTrade(uint32_t itemID, uint8_t slot, uint16_t count, const char* itemName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], itemID);
        buf[4] = slot;
        WriteU16(&buf[5], count);
        uint16_t nameLen = (uint16_t)(strlen(itemName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[7], itemName, nameLen);
        SendMsg(PipeMsg::ITEM_TRADE, buf, 7 + nameLen);
    }

    static void SendItemUpgrade(uint32_t itemID, bool success, const char* itemName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], itemID);
        buf[4] = success ? 1 : 0;
        uint16_t nameLen = (uint16_t)(strlen(itemName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[5], itemName, nameLen);
        SendMsg(PipeMsg::ITEM_UPGRADE, buf, 5 + nameLen);
    }

    static void SendItemMove(uint32_t itemID, uint8_t fromSlot, uint8_t toSlot, const char* itemName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], itemID);
        buf[4] = fromSlot;
        buf[5] = toSlot;
        uint16_t nameLen = (uint16_t)(strlen(itemName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[6], itemName, nameLen);
        SendMsg(PipeMsg::ITEM_MOVE, buf, 6 + nameLen);
    }

    // ====================================================================
    //  SKILL
    // ====================================================================

    static void SendSkillUse(uint32_t skillID, uint16_t targetID, uint16_t srcID, const char* skillName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], skillID);
        WriteU16(&buf[4], targetID);
        WriteU16(&buf[6], srcID);
        uint16_t nameLen = (uint16_t)(strlen(skillName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[8], skillName, nameLen);
        SendMsg(PipeMsg::SKILL_USE, buf, 8 + nameLen);
    }

    static void SendSkillRecv(uint32_t skillID, uint16_t targetID, uint16_t srcID, const char* skillName = "")
    {
        uint8_t buf[512];
        WriteU32(&buf[0], skillID);
        WriteU16(&buf[4], targetID);
        WriteU16(&buf[6], srcID);
        uint16_t nameLen = (uint16_t)(strlen(skillName) + 1);
        if (nameLen > 500) nameLen = 500;
        memcpy(&buf[8], skillName, nameLen);
        SendMsg(PipeMsg::SKILL_RECV, buf, 8 + nameLen);
    }

    // ====================================================================
    //  SAVAS
    // ====================================================================

    static void SendAttack(uint16_t srcID, uint16_t targetID, int32_t damage)
    {
        uint8_t buf[8];
        WriteU16(&buf[0], srcID);
        WriteU16(&buf[2], targetID);
        WriteI32(&buf[4], damage);
        SendMsg(PipeMsg::ATTACK, buf, 8);
    }

    static void SendDead(uint16_t who, const char* killerName = "")
    {
        uint8_t buf[256];
        WriteU16(&buf[0], who);
        uint16_t nameLen = (uint16_t)(strlen(killerName) + 1);
        if (nameLen > 250) nameLen = 250;
        memcpy(&buf[2], killerName, nameLen);
        SendMsg(PipeMsg::DEAD, buf, 2 + nameLen);
    }

    static void SendRegen()
    {
        SendMsg(PipeMsg::REGEN, nullptr, 0);
    }

    // ====================================================================
    //  ZONE
    // ====================================================================

    static void SendZoneChange(uint8_t oldZone, uint8_t newZone)
    {
        uint8_t buf[2] = { oldZone, newZone };
        SendMsg(PipeMsg::ZONE_CHANGE, buf, 2);
    }

    // ====================================================================
    //  CHAT
    // ====================================================================

    static void SendChat(uint8_t chatType, const char* message)
    {
        uint8_t buf[1024];
        buf[0] = chatType;
        uint16_t msgLen = (uint16_t)(strlen(message) + 1);
        if (msgLen > 1020) msgLen = 1020;
        memcpy(&buf[1], message, msgLen);
        SendMsg(PipeMsg::CHAT, buf, 1 + msgLen);
    }

    // ====================================================================
    //  GUVENLIK
    // ====================================================================

    static void SendSecurity(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::SECURITY, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    static void SendHook(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::HOOK, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    // ====================================================================
    //  HATA / UYARI
    // ====================================================================

    static void SendError(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::ERR, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    static void SendWarning(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::WARNING, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    // ====================================================================
    //  TICARET / BANKA / PUS
    // ====================================================================

    static void SendMerchant(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::MERCHANT, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    static void SendWarehouse(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::WAREHOUSE, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    static void SendPUS(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::PUS, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    // ====================================================================
    //  PARTI / KLAN / QUEST
    // ====================================================================

    static void SendParty(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::PARTY, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    static void SendClan(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::CLAN, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    static void SendQuest(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        SendMsg(PipeMsg::QUEST, (const uint8_t*)buf, (uint16_t)(strlen(buf) + 1));
    }

    // ====================================================================
    //  PERIYODIK DURUM RAPORU
    // ====================================================================

    static void SendStatus(int32_t hp, int32_t maxHp, int32_t mp, int32_t maxMp, uint32_t gold, uint16_t level, uint8_t zone)
    {
        uint8_t buf[23];
        WriteI32(&buf[0], hp);
        WriteI32(&buf[4], maxHp);
        WriteI32(&buf[8], mp);
        WriteI32(&buf[12], maxMp);
        WriteU32(&buf[16], gold);
        WriteU16(&buf[20], level);
        buf[22] = zone;
        SendMsg(PipeMsg::STATUS, buf, 23);
    }

    // ====================================================================
    //  HEARTBEAT
    // ====================================================================

    static void SendHeartbeat()
    {
        SendMsg(PipeMsg::HEARTBEAT, nullptr, 0);
    }
};
