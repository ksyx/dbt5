/*
 * Legal Notice
 *
 * This document and associated source code (the "Work") is a preliminary
 * version of a benchmark specification being developed by the TPC. The
 * Work is being made available to the public for review and comment only.
 * The TPC reserves all right, title, and interest to the Work as provided
 * under U.S. and international laws, including without limitation all patent
 * and trademark rights therein.
 *
 * No Warranty
 *
 * 1.1 TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THE INFORMATION
 *     CONTAINED HEREIN IS PROVIDED "AS IS" AND WITH ALL FAULTS, AND THE
 *     AUTHORS AND DEVELOPERS OF THE WORK HEREBY DISCLAIM ALL OTHER
 *     WARRANTIES AND CONDITIONS, EITHER EXPRESS, IMPLIED OR STATUTORY,
 *     INCLUDING, BUT NOT LIMITED TO, ANY (IF ANY) IMPLIED WARRANTIES,
 *     DUTIES OR CONDITIONS OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
 *     PURPOSE, OF ACCURACY OR COMPLETENESS OF RESPONSES, OF RESULTS, OF
 *     WORKMANLIKE EFFORT, OF LACK OF VIRUSES, AND OF LACK OF NEGLIGENCE.
 *     ALSO, THERE IS NO WARRANTY OR CONDITION OF TITLE, QUIET ENJOYMENT,
 *     QUIET POSSESSION, CORRESPONDENCE TO DESCRIPTION OR NON-INFRINGEMENT
 *     WITH REGARD TO THE WORK.
 * 1.2 IN NO EVENT WILL ANY AUTHOR OR DEVELOPER OF THE WORK BE LIABLE TO
 *     ANY OTHER PARTY FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO THE
 *     COST OF PROCURING SUBSTITUTE GOODS OR SERVICES, LOST PROFITS, LOSS
 *     OF USE, LOSS OF DATA, OR ANY INCIDENTAL, CONSEQUENTIAL, DIRECT,
 *     INDIRECT, OR SPECIAL DAMAGES WHETHER UNDER CONTRACT, TORT, WARRANTY,
 *     OR OTHERWISE, ARISING IN ANY WAY OUT OF THIS OR ANY OTHER AGREEMENT
 *     RELATING TO THE WORK, WHETHER OR NOT SUCH AUTHOR OR DEVELOPER HAD
 *     ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 * Contributors
 * - Sergey Vasilevskiy
 */

/*
*	Class representing the News Item and News XRef tables.
*/
#ifndef NEWS_ITEM_AND_XREG_TABLE_H
#define NEWS_ITEM_AND_XREG_TABLE_H

#include "EGenTables_stdafx.h"

namespace TPCE
{

const int	iNewsItemsPerCompany = 2;
const int	iNewsItemMaxDaysAgo = 50;	// how many days ago can a news item be dated

typedef struct NEWS_ITEM_AND_XREF_ROW
{
	NEWS_ITEM_ROW		news_item;
	NEWS_XREF_ROW		news_xref;
} *PNEWS_ITEM_AND_XREF_ROW;

class CNewsItemAndXRefTable : public TableTemplate<NEWS_ITEM_AND_XREF_ROW>
{
	CCompanyTable					m_CompanyTable;
	TNewsFile*						m_pNews;
	TLastNamesFile*					m_pLastNames;
	int								m_iNewsItemsGeneratedForCompany;	

	void GenerateNewsItemHeadlineAndSummary(NEWS_ITEM_ROW &news_item)
	{
		int iThreshold;
		int	iLen = 0;
		char *szWord;

		while (iLen < sizeof(news_item.NI_ITEM) - 1)
		{
			iThreshold = m_rnd.RndIntRange(0, m_pNews->GetGreatestKey() - 1);

			szWord = (m_pNews->GetRecord(iThreshold))->WORD;

			while (szWord && *szWord && (iLen < sizeof(news_item.NI_ITEM) - 1))
			{
				news_item.NI_ITEM[iLen++] = *szWord++;	// copy one letter at a time				
			}

			if (iLen < sizeof(news_item.NI_ITEM) - 1)
			{
				news_item.NI_ITEM[iLen++] = ' ';	// add space at the end of a word
			}
		}

		news_item.NI_ITEM[iLen] = '\0';	// NULL terminate in case if the last word was copied only partially

		// Now copy the headline and summary from the generated item.		
		memcpy(news_item.NI_HEADLINE, news_item.NI_ITEM, sizeof(news_item.NI_HEADLINE)-1);
		// news_item.NI_HEADLINE will be zero-terminated because it is initialized to all 0s
		// in TableTemplate constructor and the last character is not overwritten.

		// Now copy the headline and summary from the generated item.		
		memcpy(news_item.NI_SUMMARY, news_item.NI_ITEM, sizeof(news_item.NI_SUMMARY)-1);
		// news_item.NI_SUMMARY will be zero-terminated because it is initialized to all 0s
		// in TableTemplate constructor and the last character is not overwritten.
	}

public:
	CNewsItemAndXRefTable(	CInputFiles inputFiles,
							TIdent		iCustomerCount, 
							TIdent		iStartFromCustomer)
		: m_CompanyTable(inputFiles, iCustomerCount, iStartFromCustomer)
		, m_pNews(inputFiles.News)
		, m_pLastNames(inputFiles.LastNames)
		, m_iNewsItemsGeneratedForCompany(0)
	{		
		m_iLastRowNumber = iNewsItemsPerCompany * inputFiles.Company->CalculateStartFromCompany(iStartFromCustomer);		
	};

	/*
	*	Generates all column values for the next row.
	*/
	bool GenerateNextRecord()
	{
		int	iAddDayNo, iAddMSec, iThreshold;		
		
		// Generate NEWS_ITEM row
		m_row.news_item.NI_ID = m_iLastRowNumber + 1;	// row number starts from 0
		GenerateNewsItemHeadlineAndSummary(m_row.news_item);
		iAddDayNo = m_rnd.RndIntRange(0, iNewsItemMaxDaysAgo);
		iAddMSec = m_rnd.RndIntRange(0, MsPerDay);
		m_row.news_item.NI_DTS.SetToCurrent();	// substruct from today's date
		m_row.news_item.NI_DTS.Add((-1) * iAddDayNo, (-1) * iAddMSec);

		iThreshold = m_rnd.RndIntRange(0, m_pLastNames->GetGreatestKey() - 1);
		strncpy(m_row.news_item.NI_AUTHOR, (m_pLastNames->GetRecord(iThreshold))->LAST_NAME, 
			sizeof(m_row.news_item.NI_AUTHOR)-1);

		iThreshold = m_rnd.RndIntRange(0, m_pLastNames->GetGreatestKey() - 1);
		strncpy(m_row.news_item.NI_SOURCE, (m_pLastNames->GetRecord(iThreshold))->LAST_NAME,
			sizeof(m_row.news_item.NI_SOURCE)-1);

		// Generate NEWS_XREF row
		m_row.news_xref.NX_NI_ID = m_row.news_item.NI_ID;
		m_row.news_xref.NX_CO_ID = m_CompanyTable.GetCurrentCO_ID();

		++m_iNewsItemsGeneratedForCompany;

		++m_iLastRowNumber;		

		if (m_iNewsItemsGeneratedForCompany >= iNewsItemsPerCompany)
		{
			m_bMoreRecords = m_CompanyTable.GenerateNextCO_ID();
			m_iNewsItemsGeneratedForCompany = 0;
		}
		else
		{
			m_bMoreRecords = true;	// need to generate more rows for at least the current company
		}

		return (MoreRecords());
		//return (m_iLastRowNumber < 10);
		
	};

	PNEWS_ITEM_ROW GetNewsItemRow() { return &m_row.news_item; }
	PNEWS_XREF_ROW GetNewsXRefRow() { return &m_row.news_xref; }
};

}	// namespace TPCE

#endif //NEWS_ITEM_AND_XREG_TABLE_H