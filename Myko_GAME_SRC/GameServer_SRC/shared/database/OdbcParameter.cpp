#include "stdafx.h"
#include "OdbcConnection.h"

OdbcParameter::OdbcParameter(SQLSMALLINT parameterType, SQLSMALLINT dataType, SQLPOINTER parameterAddress, SQLLEN maxLength /*= 1*/)
	: m_parameterType(parameterType), m_cDataType(dataType), m_parameterAddress(parameterAddress), 
	m_dataTypeLength(0), m_pCBValue(SQL_NTS)
{
	switch (m_cDataType)
	{
	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_BINARY:
	case SQL_VARBINARY:
		m_dataTypeLength = maxLength;
		m_dataType = m_cDataType;
		if (m_cDataType == SQL_BINARY || m_cDataType == SQL_CHAR)
			m_pCBValue = m_dataTypeLength;
		break;

	case SQL_C_STINYINT:
	case SQL_C_UTINYINT:
		m_dataType = SQL_TINYINT;
		m_dataTypeLength = 3;
		break;

	case SQL_C_SSHORT:
	case SQL_C_USHORT:
		m_dataType = SQL_SMALLINT;
		m_dataTypeLength = 5;
		break;

	case SQL_C_SLONG:
	case SQL_C_ULONG:
		m_dataType = SQL_INTEGER;
		m_dataTypeLength = 10;
		break;

	case SQL_C_FLOAT: // = SQL_REAL = 7 — OdbcCommand SQL_C_FLOAT ile cagiriyor
		m_dataType = SQL_REAL;
		m_cDataType = SQL_C_FLOAT;
		m_dataTypeLength = 24;
		break;

	case SQL_C_DOUBLE: // = SQL_DOUBLE = 8
		m_dataType = SQL_DOUBLE;
		m_cDataType = SQL_C_DOUBLE;
		m_dataTypeLength = 53;
		break;

	case SQL_C_SBIGINT: // = -25
		m_dataType = SQL_BIGINT;
		m_cDataType = SQL_C_SBIGINT;
		m_dataTypeLength = 19;
		break;

	case SQL_C_UBIGINT: // = -27
		m_dataType = SQL_BIGINT;
		m_cDataType = SQL_C_UBIGINT;
		m_dataTypeLength = 20;
		break;

	case SQL_C_BIT: // = SQL_BIT = -7
		m_dataType = SQL_BIT;
		m_cDataType = SQL_C_BIT;
		m_dataTypeLength = 1;
		break;

	default: // unknown, default to integer
		m_dataType = SQL_INTEGER;
		m_cDataType = m_dataType + SQL_SIGNED_OFFSET;
		m_dataTypeLength = 10;
	}
}
