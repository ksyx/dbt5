/*
*	(c) Copyright 2002-2003, Microsoft Corporation
*	Provided to the TPC under license.
*	Written by Sergey Vasilevskiy.
*/

/*
*	Class representing the DAILY_MARKET table.
*/
#ifndef DAILY_MARKET_TABLE_H
#define DAILY_MARKET_TABLE_H

#include "EGenTables_stdafx.h"
#include "SecurityPriceRange.h"

namespace TPCE
{

const int iTradeDaysInYear = 261;	//the number of trading days in a year (for DAILY_MARKET)
const int iDailyMarketYears = 5;	//number of years of history in DAILY_MARKET
const int iDailyMarketTotalRows = iDailyMarketYears * iTradeDaysInYear;

const double	fDailyMarketCloseMin = fMinSecPrice;
const double	fDailyMarketCloseMax = fMaxSecPrice;

const double	fDailyMarketHighRelativeToClose = 1.05;
const double	fDailyMarketLowRelativeToClose = 0.92;

const int		iDailyMarketVolumeMax = 10000;
const int		iDailyMarketVolumeMin = 1000;

typedef struct DAILY_MARKET_GEN_ROW 
{	
	//big array of all the history for one security
	DAILY_MARKET_ROW	m_daily_market[iDailyMarketTotalRows];
} *PDAILY_MARKET_GEN_ROW;

class CDailyMarketTable : public TableTemplate<DAILY_MARKET_GEN_ROW>
{	
	TIdent			m_iStartFromSecurity;	
	TIdent			m_iSecurityCount;
	CSecurityFile*	m_SecurityFile;
	CDateTime		m_StartFromDate;
	int				m_iDailyMarketTotalRows;
	// Number of times GenerateNextRecord() was called for the current security data.
	// Needed to decide when to generate next security's data.
	int				m_iRowsGeneratedPerSecurity;
	// Stores whether there is another security(s) for which to 
	// generate daily market data.
	bool			m_bMoreSecurities;	

	/*
	*	DAILY_MARKET table rows generation
	*/
	void GenerateDailyMarketRows()
	{
		int		i;
		int		iDayNo = 0;	//start from the oldest date (start date)
		char	szSymbol[cSYMBOL_len + 1];

		// Create symbol only once.
		//
		m_SecurityFile->CreateSymbol(m_iLastRowNumber, szSymbol, sizeof(szSymbol));

		for (i = 0; i < m_iDailyMarketTotalRows; ++i)
		{
			//copy the symbol
			strncpy(m_row.m_daily_market[i].DM_S_SYMB, szSymbol, sizeof(m_row.m_daily_market[i].DM_S_SYMB)-1);

			//generate trade date
			m_row.m_daily_market[i].DM_DATE = m_StartFromDate;
			m_row.m_daily_market[i].DM_DATE.Add(iDayNo, 0);

			//generate prices
			m_row.m_daily_market[i].DM_CLOSE = m_rnd.RndDoubleIncrRange(fDailyMarketCloseMin, fDailyMarketCloseMax, 0.01);
			m_row.m_daily_market[i].DM_HIGH = m_row.m_daily_market[i].DM_CLOSE * fDailyMarketHighRelativeToClose;
			m_row.m_daily_market[i].DM_LOW = m_row.m_daily_market[i].DM_CLOSE * fDailyMarketLowRelativeToClose;

			//generate volume
			m_row.m_daily_market[i].DM_VOL = m_rnd.RndIntRange(iDailyMarketVolumeMin, iDailyMarketVolumeMax);

			++iDayNo;	//go one day forward for the next row

			if ((iDayNo % 7) == 5) iDayNo += 2;  // skip weekend
		}
	}

public:
	/*
	*	Constructor.
	*/
	CDailyMarketTable(	CInputFiles inputFiles, 
						TIdent		iCustomerCount, 
						TIdent		iStartFromCustomer)
		: TableTemplate<DAILY_MARKET_GEN_ROW>()
		, m_SecurityFile(inputFiles.Securities)
		, m_iDailyMarketTotalRows(sizeof(m_row.m_daily_market)/sizeof(m_row.m_daily_market[0]))
		, m_iRowsGeneratedPerSecurity(sizeof(m_row.m_daily_market)/sizeof(m_row.m_daily_market[0]))		
		, m_bMoreSecurities(true)	// initialize once
	{		
		//	Set DAILY_MARKET start date to Jan 03, 2000.
		//
		m_StartFromDate.Set(iDailyMarketBaseYear, iDailyMarketBaseMonth, 
							iDailyMarketBaseDay, iDailyMarketBaseHour, 
							iDailyMarketBaseMinute, iDailyMarketBaseSecond, iDailyMarketBaseMsec);
		
		m_bMoreRecords = true;		// initialize once

		m_iSecurityCount = m_SecurityFile->CalculateSecurityCount(iCustomerCount);
		m_iStartFromSecurity = m_SecurityFile->CalculateStartFromSecurity(iStartFromCustomer);

		m_iLastRowNumber = m_iStartFromSecurity;
	};

	bool GenerateNextRecord()
	{	
		++m_iRowsGeneratedPerSecurity;

		if (m_iRowsGeneratedPerSecurity >= m_iDailyMarketTotalRows)
		{
			if (m_bMoreSecurities)
			{
				GenerateDailyMarketRows();	// generate all rows for the current security

				++m_iLastRowNumber;
			
				//Update state info
				m_bMoreSecurities = m_iLastRowNumber < (m_iStartFromSecurity + m_iSecurityCount);

				m_iRowsGeneratedPerSecurity = 0;
			}
		}

		// Return false when generated the last row of the last security
		if (!m_bMoreSecurities && (m_iRowsGeneratedPerSecurity == m_iDailyMarketTotalRows - 1))
		{
			m_bMoreRecords = false;
		}

		return (MoreRecords());
	}

	const PDAILY_MARKET_ROW GetRow()
	{
		return &m_row.m_daily_market[m_iRowsGeneratedPerSecurity];
	}	
};

}	// namespace TPCE

#endif //DAILY_MARKET_TABLE_H