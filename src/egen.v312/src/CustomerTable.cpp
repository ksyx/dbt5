/*
*	(c) Copyright 2002-2003, Microsoft Corporation
*	Provided to the TPC under license.
*	Written by Sergey Vasilevskiy.
*/

#include "../inc/EGenTables_stdafx.h"

using namespace TPCE;

const char *szUSAreaCode="011";	//USA/Canada phone area code

// Percentages used when generating C_TIER
const int	iPercentCustomersInC_TIER_1 = 20;
const int	iPercentCustomersInC_TIER_2 = 60;
const int	iPercentCustomersInC_TIER_3 = 100 - iPercentCustomersInC_TIER_1 - iPercentCustomersInC_TIER_2;

// Percentages used when generating C_DOB
const int	iPercentUnder18			= 5;
const int	iPercentBetween19And24	= 16;
const int	iPercentBetween25And34	= 17;
const int	iPercentBetween35And44	= 19;
const int	iPercentBetween45And54	= 16;
const int	iPercentBetween55And64	= 11;
const int	iPercentBetween65And74	= 8;
const int	iPercentBetween75And84	= 7;
const int	iPercentOver85			= 1;

// Number of RNG calls to skip for one row in order
// to not use any of the random values from the previous row.
const int iRNGSkipOneRowCustomer = 35;	// real max count in v3.5: 29

/*
*	CustomerTable constructor
*/
CCustomerTable::CCustomerTable(CInputFiles	inputFiles, 
							   TIdent		iCustomerCount, 
							   TIdent		iStartFromCustomer)
	: TableTemplate<CUSTOMER_ROW>()
	, m_iRowsToGenerate(iCustomerCount)
	, m_person(inputFiles)
	, m_Phones(inputFiles.AreaCodes)
	, m_StatusTypeFile(inputFiles.StatusType)
{
	m_row.C_ID = iStartFromCustomer - 1;	//initialize to the starting customer number (1-based)

	m_iCompanyCount = inputFiles.Company->GetSize();
	m_iExchangeCount = inputFiles.Exchange->GetSize();	
}

/*
*	Reset the state for the next load unit
*/
void CCustomerTable::InitNextLoadUnit()
{
	m_rnd.SetSeed(m_rnd.RndNthElement(RNGSeedTableDefault,
									  m_row.C_ID * iRNGSkipOneRowCustomer));
}

/*
*	Generates only the next Customer ID value
*/
TIdent CCustomerTable::GenerateNextC_ID()
{	
	if (m_row.C_ID % iDefaultLoadUnitSize == 0)
	{
		InitNextLoadUnit();
	}

	++m_iLastRowNumber;	//increment state info
	m_bMoreRecords = m_iLastRowNumber < m_iRowsToGenerate;
	return ++m_row.C_ID;	//sequential for now
}

/*
*	Generate tax id.
*/
void CCustomerTable::GetC_TAX_ID(TIdent C_ID, char *szOutput)
{		
	m_person.GetTaxID(C_ID, szOutput);
}

/*
*	Generate C_ST_ID.
*/
void CCustomerTable::GenerateC_ST_ID()
{
	strncpy(m_row.C_ST_ID, m_StatusTypeFile->GetRecord(eActive)->ST_ID, sizeof(m_row.C_ST_ID)-1);
}

/*
*	Generate first, last name, and the gender.
*/
void CCustomerTable::GeneratePersonInfo()
{
	//Fill in the first name, last name, and the tax id
	m_person.GetFirstLastAndTaxID(m_row.C_ID, m_row.C_F_NAME, m_row.C_L_NAME, m_row.C_TAX_ID);
	//Fill in the gender
	m_row.C_GNDR = m_person.GetGender(m_row.C_ID);	
	//Fill in the middle name
	m_row.C_M_NAME[0] = m_person.GetMiddleName(m_row.C_ID);
	m_row.C_M_NAME[1] = '\0';	
}

/*
*	Generate C_TIER.
*/
eCustomerTier CCustomerTable::GetC_TIER(TIdent C_ID)
{
	return m_CustomerSelection.GetTier(C_ID);
}

/*
*	Generate date of birth.
*/
void CCustomerTable::GenerateC_DOB()
{
	//min and max age brackets limits in years
	static int age_brackets[]={10, 19, 25, 35, 45, 55, 65, 75, 85, 100};
	int age_bracket;
	int year, month, day;	//current date
	int	dob_daysno_min, dob_daysno_max;	//min and max date of birth in days
	int dob_in_days;	//generated random date of birth in days

	int iThreshold = m_rnd.RndGenerateIntegerPercentage();

//Determine customer age bracket according to the distribution.
	if (iThreshold <= iPercentUnder18)
		age_bracket = 0;
	else
		if (iThreshold <= iPercentUnder18+iPercentBetween19And24)
			age_bracket = 1;
		else
			if (iThreshold <= iPercentUnder18+iPercentBetween19And24+iPercentBetween25And34)
				age_bracket = 2;
			else
				if (iThreshold <= iPercentUnder18+iPercentBetween19And24+iPercentBetween25And34+
					iPercentBetween35And44)
					age_bracket = 3;
				else
					if (iThreshold <= iPercentUnder18+iPercentBetween19And24+iPercentBetween25And34+
						iPercentBetween35And44+iPercentBetween45And54)
						age_bracket = 4;
					else
						if (iThreshold <= iPercentUnder18+iPercentBetween19And24+iPercentBetween25And34+
							iPercentBetween35And44+iPercentBetween45And54+iPercentBetween55And64)
							age_bracket = 5;
						else
							if (iThreshold <= iPercentUnder18+iPercentBetween19And24+iPercentBetween25And34+
								iPercentBetween35And44+iPercentBetween45And54+iPercentBetween55And64+iPercentBetween65And74)
								age_bracket = 6;
							else
								if (iThreshold <= iPercentUnder18+iPercentBetween19And24+iPercentBetween25And34+
									iPercentBetween35And44+iPercentBetween45And54+iPercentBetween55And64+iPercentBetween65And74+iPercentBetween75And84)
									age_bracket = 7;
								else
									age_bracket = 8;
	assert(age_bracket < sizeof(age_brackets)/sizeof(age_brackets[0]));

	//Get current date
	m_date_time.GetYMD(&year, &month, &day);
	//Check for today to be a leap date
	if (month==2 && day==29)	//February 29 ?
	{
		month = 3; day = 1;	//Set today to March 1st.
	}

	dob_daysno_min = CDateTime::YMDtoDayno(year - age_brackets[age_bracket+1], month, day)-1;
	dob_daysno_max = CDateTime::YMDtoDayno(year - age_brackets[age_bracket], month, day);

	//Generate the random age expressed in days that falls into the particular range.
	dob_in_days = m_rnd.RndIntRange(dob_daysno_min, dob_daysno_max);

	m_row.C_DOB.Set(dob_in_days);
}

/*
*	Generate C_AD_ID (address id).
*/
void CCustomerTable::GenerateC_AD_ID()
{
	// Generate address id sequentially, allowing space for Exchanges and Companies
	// in the beggining of the address ids range.
	m_row.C_AD_ID = m_iExchangeCount + m_iCompanyCount + m_row.C_ID;
}

/*
*	Generate C_CTRY_1.
*/
void CCustomerTable::GenerateC_CTRY_1()
{
	strncpy(m_row.C_CTRY_1, szUSAreaCode, sizeof(m_row.C_CTRY_1)-1);
	m_row.C_CTRY_1[sizeof(m_row.C_CTRY_1)-1] = '\0';	//null terminate
}

/*
*	Generate C_CTRY_2.
*/
void CCustomerTable::GenerateC_CTRY_2()
{
	strncpy(m_row.C_CTRY_2, szUSAreaCode, sizeof(m_row.C_CTRY_2)-1);
	m_row.C_CTRY_2[sizeof(m_row.C_CTRY_2)-1] = '\0';	//null terminate
}

/*
*	Generate C_CTRY_3.
*/
void CCustomerTable::GenerateC_CTRY_3()
{
	strncpy(m_row.C_CTRY_3, szUSAreaCode, sizeof(m_row.C_CTRY_3)-1);
	m_row.C_CTRY_3[sizeof(m_row.C_CTRY_3)-1] = '\0';	//null terminate
}


/*
*	Generate C_AREA_1
*/
void CCustomerTable::GenerateC_AREA_1()
{
	RNGSEED	OldSeed;
	int		iThreshold;

	OldSeed = m_rnd.GetSeed();

	m_rnd.SetSeed( m_rnd.RndNthElement( RNGSeedBaseC_AREA_1, m_row.C_ID ));

	//generate Threshold up to the value of the last key (first member in a pair)
	iThreshold = m_rnd.RndIntRange(0, m_Phones->GetGreatestKey() - 1);

	//copy the area code that corresponds to the Threshold
	strncpy(m_row.C_AREA_1, (m_Phones->GetRecord(iThreshold))->AREA_CODE, sizeof(m_row.C_AREA_1)-1);
	m_row.C_AREA_1[sizeof(m_row.C_AREA_1)-1] = '\0';	//null terminate

	m_rnd.SetSeed( OldSeed );
}

/*
*	Generate C_AREA_2
*/
void CCustomerTable::GenerateC_AREA_2()
{
	RNGSEED	OldSeed;
	int		iThreshold;

	OldSeed = m_rnd.GetSeed();

	m_rnd.SetSeed( m_rnd.RndNthElement( RNGSeedBaseC_AREA_2, m_row.C_ID ));

	//generate Threshold up to the value of the last key (first member in a pair)
	iThreshold = m_rnd.RndIntRange(0, m_Phones->GetGreatestKey() - 1);

	//copy the area code that corresponds to the Threshold
	strncpy(m_row.C_AREA_2, (m_Phones->GetRecord(iThreshold))->AREA_CODE, sizeof(m_row.C_AREA_2)-1);
	m_row.C_AREA_2[sizeof(m_row.C_AREA_2)-1] = '\0';	//null terminate

	m_rnd.SetSeed( OldSeed );
}

/*
*	Generate C_AREA_3
*/
void CCustomerTable::GenerateC_AREA_3()
{
	RNGSEED	OldSeed;
	int		iThreshold;

	OldSeed = m_rnd.GetSeed();

	m_rnd.SetSeed( m_rnd.RndNthElement( RNGSeedBaseC_AREA_3, m_row.C_ID ));

	//generate Threshold up to the value of the last key (first member in a pair)
	iThreshold = m_rnd.RndIntRange(0, m_Phones->GetGreatestKey() - 1);

	//copy the area code that corresponds to the Threshold
	strncpy(m_row.C_AREA_3, (m_Phones->GetRecord(iThreshold))->AREA_CODE, sizeof(m_row.C_AREA_3)-1);
	m_row.C_AREA_3[sizeof(m_row.C_AREA_3)-1] = '\0';	//null terminate

	m_rnd.SetSeed( OldSeed );
}


/*
*	Generate C_LOCAL_1.
*/
void CCustomerTable::GenerateC_LOCAL_1()
{
	m_rnd.RndAlphaNumFormatted(m_row.C_LOCAL_1, (char*)"nnnnnnn");	//7-digit phone number
}


/*
*	Generate C_LOCAL_2.
*/
void CCustomerTable::GenerateC_LOCAL_2()
{
	m_rnd.RndAlphaNumFormatted(m_row.C_LOCAL_2, (char*)"nnnnnnn");	//7-digit phone number
}

/*
*	Generate C_LOCAL_3.
*/
void CCustomerTable::GenerateC_LOCAL_3()
{
	m_rnd.RndAlphaNumFormatted(m_row.C_LOCAL_3, (char*)"nnnnnnn");	//7-digit phone number
}


/*
*	Generate C_EXT_1.
*/
void CCustomerTable::GenerateC_EXT_1()
{
	int iThreshold = m_rnd.RndGenerateIntegerPercentage();

	if (iThreshold <= 25)
	{
		m_rnd.RndAlphaNumFormatted(m_row.C_EXT_1, (char*)"nnn");	//3-digit phone extension
	}
	else
	{
		*m_row.C_EXT_1 = '\0';	//no extension
	}
}

/*
*	Generate C_EXT_2.
*/
void CCustomerTable::GenerateC_EXT_2()
{
	int iThreshold = m_rnd.RndGenerateIntegerPercentage();

	if (iThreshold <= 15)
	{
		m_rnd.RndAlphaNumFormatted(m_row.C_EXT_2, (char*)"nnn");	//3-digit phone extension
	}
	else
	{
		*m_row.C_EXT_2 = '\0';	//no extension
	}
}

/*
*	Generate C_EXT_3.
*/
void CCustomerTable::GenerateC_EXT_3()
{
	int iThreshold = m_rnd.RndGenerateIntegerPercentage();

	if (iThreshold <= 5)
	{
		m_rnd.RndAlphaNumFormatted(m_row.C_EXT_3, (char*)"nnn");	//3-digit phone extension
	}
	else
	{
		*m_row.C_EXT_3 = '\0';	//no extension
	}
}


/*
*	Generate Email 1 and Email 2 that are guaranteed to be different.
*/
void CCustomerTable::GenerateC_EMAIL_1_and_C_EMAIL_2()
{
	size_t	iLen;
	int		iEmail1Index;

	iEmail1Index = m_rnd.RndIntRange(0, iNumEMAIL_DOMAINs - 1);

	// Generate EMAIL_1
	iLen = strlen(m_row.C_L_NAME);
	m_row.C_EMAIL_1[0] = m_row.C_F_NAME[0];	//first char of the first name
	strncpy(&m_row.C_EMAIL_1[1], m_row.C_L_NAME, sizeof(m_row.C_EMAIL_1) - 2); //last name
	strncpy(&m_row.C_EMAIL_1[1+iLen], EMAIL_DOMAINs[iEmail1Index], //domain name
			sizeof(m_row.C_EMAIL_1) - iLen - 2);
	m_row.C_EMAIL_1[sizeof(m_row.C_EMAIL_1)-1] = '\0';	//null terminate in any case

	// Generate EMAIL_2 that is different from EMAIL_1
	size_t len = strlen(m_row.C_L_NAME);
	m_row.C_EMAIL_2[0] = m_row.C_F_NAME[0];	//first char of the first name
	strncpy(&m_row.C_EMAIL_2[1], m_row.C_L_NAME, sizeof(m_row.C_EMAIL_2) - 2); //last name
	strncpy(&m_row.C_EMAIL_2[1+iLen], EMAIL_DOMAINs[m_rnd.RndIntRangeExclude(0, iNumEMAIL_DOMAINs - 1, iEmail1Index)],	//domain name
			sizeof(m_row.C_EMAIL_2) - iLen - 2);	
	m_row.C_EMAIL_2[sizeof(m_row.C_EMAIL_2)-1] = '\0';	//null terminate in any case
}

/*
*	Generates all column values for the next row
*/
bool CCustomerTable::GenerateNextRecord()
{
	GenerateNextC_ID();	
	GetC_TAX_ID(m_row.C_ID, m_row.C_TAX_ID);
	GenerateC_ST_ID();
	GeneratePersonInfo();	//generate last name, first name, and gender.
	m_row.C_TIER = GetC_TIER(m_row.C_ID);
	GenerateC_DOB();
	GenerateC_AD_ID();
	GenerateC_CTRY_1();
	GenerateC_AREA_1();
	GenerateC_LOCAL_1();
	GenerateC_EXT_1();	
	GenerateC_CTRY_2();
	GenerateC_AREA_2();
	GenerateC_LOCAL_2();	
	GenerateC_EXT_2();
	GenerateC_CTRY_3();
	GenerateC_AREA_3();
	GenerateC_LOCAL_3();	
	GenerateC_EXT_3();
	GenerateC_EMAIL_1_and_C_EMAIL_2();

	//Return false if all the rows have been generated
	return MoreRecords();
}