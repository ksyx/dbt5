/*
 * TxnBaseDB.cpp
 *
 * 2006 Rilson Nascimento
 *
 * 13 June 2006
 */

#include "../include/transactions.h"

using namespace TPCE;

CTxnBaseDB::CTxnBaseDB(CDBConnection *pDBConn)
: m_pDBConnection(pDBConn)
{
	m_Conn = m_pDBConnection->m_Conn;  //FIXME?
	m_Txn = m_pDBConnection->m_Txn;
}

CTxnBaseDB::~CTxnBaseDB()
{
}

void CTxnBaseDB::BeginTxn()
{
	m_pDBConnection->BeginTxn();
}

void CTxnBaseDB::CommitTxn()
{
	m_pDBConnection->CommitTxn();
}

void CTxnBaseDB::RollbackTxn()
{
	m_pDBConnection->RollbackTxn();
}