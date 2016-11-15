/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)
*/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "linphone/core_utils.h"

/*
 * http://en.wikipedia.org/wiki/Telephone_numbering_plan
 * http://en.wikipedia.org/wiki/Telephone_numbers_in_Europe
 * imported from https://en.wikipedia.org/wiki/List_of_mobile_phone_number_series_by_country
 */
static LinphoneDialPlan const dial_plans[]={
	//Country					, iso country code, e164 country calling code, number length, international usual prefix
	{"Afghanistan"                  ,"AF"		, "93"      , 9		, "00"  },
	{"Albania"                      ,"AL"		, "355"     , 9		, "00"  },
	{"Algeria"                      ,"DZ"		, "213"     , 9		, "00"  },
	{"American Samoa"               ,"AS"		, "1"       , 10	, "011"	},
	{"Andorra"                      ,"AD"		, "376"     , 6		, "00"  },
	{"Angola"                       ,"AO"		, "244"     , 9		, "00"  },
	{"Anguilla"                     ,"AI"		, "1"       , 10	, "011" },
	{"Antigua and Barbuda"          ,"AG"		, "1"       , 10	, "011"	},
	{"Argentina"                    ,"AR"		, "54"      , 10	, "00"  },
	{"Armenia"                      ,"AM"		, "374"     , 8		, "00"  },
	{"Aruba"                        ,"AW"		, "297"     , 7		, "011"	},
	{"Australia"                    ,"AU"		, "61"      , 9	    , "0011"},
	{"Austria"                      ,"AT"		, "43"      , 10	, "00"  },
	{"Azerbaijan"                   ,"AZ"       , "994"     , 9		, "00"  },
	{"Bahamas"                      ,"BS"		, "1"       , 10    , "011"	},
	{"Bahrain"                      ,"BH"		, "973"     , 8     , "00"  },
	{"Bangladesh"                   ,"BD"		, "880"     , 10    , "00"  },
	{"Barbados"                     ,"BB"		, "1"       , 10    , "011"	},
	{"Belarus"                      ,"BY"		, "375"     , 9     , "00"  },
	{"Belgium"                      ,"BE"		, "32"      , 9     , "00"  },
	{"Belize"                       ,"BZ"		, "501"     , 7     , "00"  },
	{"Benin"                        ,"BJ"		, "229"     , 8     , "00"	},
	{"Bermuda"                      ,"BM"		, "1"       , 10    , "011" },
	{"Bhutan"                       ,"BT"		, "975"     , 8     , "00"  },
	{"Bolivia"                      ,"BO"		, "591"     , 8     , "00"	},
	{"Bosnia and Herzegovina"       ,"BA"		, "387"     , 8     , "00"  },
	{"Botswana"                     ,"BW"		, "267"     , 8     , "00"  },
	{"Brazil"                       ,"BR"		, "55"      , 10	, "00"  },
	{"Brunei Darussalam"            ,"BN"		, "673"     , 7		, "00"	},
	{"Bulgaria"                     ,"BG"		, "359"     , 9		, "00"  },
	{"Burkina Faso"                 ,"BF"		, "226"     , 8		, "00"  },
	{"Burundi"                      ,"BI"		, "257"     , 8     , "011" },
	{"Cambodia"                     ,"KH"		, "855"     , 9		, "00"  },
	{"Cameroon"                     ,"CM"		, "237"     , 8		, "00"  },
	{"Canada"                       ,"CA"		, "1"       , 10	, "011" },
	{"Cape Verde"                   ,"CV"		, "238"     , 7		, "00"	},
	{"Cayman Islands"               ,"KY"		, "1"       , 10	, "011" },
	{"Central African Republic"     ,"CF"		, "236"     , 8     , "00"  },
	{"Chad"                         ,"TD"		, "235"     , 8		, "00"	},
	{"Chile"                        ,"CL"		, "56"      , 9	    , "00"  },
	{"China"                        ,"CN"		, "86"      , 11	, "00"  },
	{"Colombia"                     ,"CO"       , "57"      , 10	, "00"  },
	{"Comoros"                      ,"KM"		, "269"     , 7     , "00"	},
	{"Congo"                        ,"CG"		, "242"     , 9		, "00"	},
	{"Congo Democratic Republic"	,"CD"		, "243"     , 9		, "00"  },
	{"Cook Islands"                 ,"CK"		, "682"     , 5		, "00"  },
	{"Costa Rica"                   ,"CR"		, "506"     , 8     , "00"	},
	{"Cote d'Ivoire"	            ,"AD"		, "225"     , 8     , "00"  },
	{"Croatia"                      ,"HR"		, "385"     , 9		, "00"  },
	{"Cuba"                         ,"CU"		, "53"      , 8     , "119" },
	{"Cyprus"                       ,"CY"		, "357"     , 8     , "00"	},
	{"Czech Republic"               ,"CZ"		, "420"     , 9     , "00"  },
	{"Denmark"                      ,"DK"		, "45"      , 8		, "00"  },
	{"Djibouti"                     ,"DJ"		, "253"     , 8		, "00"	},
	{"Dominica"                     ,"DM"		, "1"       , 10	, "011" },
	{"Dominican Republic"	        ,"DO"		, "1"       , 10	, "011" },
	{"Ecuador"                      ,"EC"       , "593"     , 9		, "00"  },
	{"Egypt"                        ,"EG"		, "20"      , 10	, "00"	},
	{"El Salvador"                  ,"SV"		, "503"     , 8		, "00"	},
	{"Equatorial Guinea"            ,"GQ"		, "240"     , 9		, "00"  },
	{"Eritrea"                      ,"ER"		, "291"     , 7		, "00"  },
	{"Estonia"                      ,"EE"		, "372"     , 8     , "00"	},
	{"Ethiopia"                     ,"ET"		, "251"     , 9     , "00"  },
	{"Falkland Islands"	            ,"FK"		, "500"     , 5		, "00"  },
	{"Faroe Islands"	            ,"FO"		, "298"     , 6     , "00"  },
	{"Fiji"                         ,"FJ"		, "679"     , 7     , "00"	},
	{"Finland"                      ,"FI"		, "358"     , 9     , "00"  },
	{"France"                       ,"FR"		, "33"      , 9		, "00"	},
	{"French Guiana"				,"GF"		, "594"     , 9		, "00"	},
	{"French Polynesia"             ,"PF"		, "689"     , 6	    , "00"  },
	{"Gabon"                        ,"GA"		, "241"     , 8     , "00"  },
	{"Gambia"                       ,"GM"       , "220"     , 7		, "00"  },
	{"Georgia"                      ,"GE"		, "995"     , 9     , "00"	},
	{"Germany"                      ,"DE"		, "49"      , 11	, "00"	},
	{"Ghana"                        ,"GH"		, "233"     , 9		, "00"  },
	{"Gibraltar"                    ,"GI"		, "350"     , 8		, "00"  },
	{"Greece"                       ,"GR"		, "30"      ,10     , "00"	},
	{"Greenland"                    ,"GL"		, "299"     , 6		, "00"  },
	{"Grenada"                      ,"GD"		, "1"       , 10	, "011" },
	{"Guadeloupe"                   ,"GP"		, "590"     , 9     , "00"  },
	{"Guam"                         ,"GU"		, "1"       , 10	, "011"	},
	{"Guatemala"                    ,"GT"		, "502"     , 8     , "00"  },
	{"Guinea"                       ,"GN"		, "224"     , 8		, "00"  },
	{"Guinea-Bissau"				,"GW"		, "245"     , 7		, "00"	},
	{"Guyana"                       ,"GY"		, "592"     , 7	    , "001" },
	{"Haiti"                        ,"HT"		, "509"     , 8     , "00"  },
	{"Honduras"                     ,"HN"       , "504"     , 8		, "00"  },
	{"Hong Kong"                    ,"HK"		, "852"     , 8     , "001"	},
	{"Hungary"                      ,"HU"		, "36"      , 9     , "00"  },
	{"Iceland"                      ,"IS"		, "354"     , 9     , "00"  },
	{"India"                        ,"IN"		, "91"      , 10    , "00"  },
	{"Indonesia"                    ,"ID"		, "62"      , 10	, "001"	},
	{"Iran"                         ,"IR"		, "98"      , 10	, "00"	},
	{"Iraq"                         ,"IQ"		, "964"     , 10	, "00"  },
	{"Ireland"                      ,"IE"		, "353"     , 9		, "00"  },
	{"Israel"                       ,"IL"		, "972"     , 9     , "00"	},
	{"Italy"                        ,"IT"		, "39"      , 10	, "00"  },
/*	{"Jersey"                       ,"JE"		, "44"      , 10	, "00"	},*/
	{"Jamaica"                      ,"JM"		, "1"       , 10	, "011" },
	{"Japan"                        ,"JP"		, "81"      , 10	, "010" },
	{"Jordan"                       ,"JO"		, "962"     , 9     , "00"	},
	{"Kazakhstan"                   ,"KZ"		, "7"       , 10    , "00"  },
	{"Kenya"                        ,"KE"		, "254"     , 9		, "000" },
	{"Kiribati"                     ,"KI"		, "686"     , 5		, "00"	},
	{"Korea, North"                 ,"KP"		, "850"     , 12	, "99"  },
	{"Korea, South"                 ,"KR"       , "82"      , 12	, "001" },
	{"Kuwait"                       ,"KW"		, "965"     , 8     , "00"	},
	{"Kyrgyzstan"                   ,"KG"		, "996"     , 9     , "00"  },
	{"Laos"                         ,"LA"		, "856"     , 10    , "00"  },
	{"Latvia"                       ,"LV"		, "371"     , 8     , "00"	},
	{"Lebanon"                      ,"LB"		, "961"     , 7     , "00"	},
	{"Lesotho"                      ,"LS"		, "266"     , 8		, "00"	},
	{"Liberia"                      ,"LR"		, "231"     , 8		, "00"  },
	{"Libya"                        ,"LY"		, "218"     , 8		, "00"  },
	{"Liechtenstein"                ,"LI"		, "423"     , 7     , "00"	},
	{"Lithuania"                    ,"LT"		, "370"     , 8		, "00"  },
	{"Luxembourg"                   ,"LU"		, "352"     , 9		, "00"  },
	{"Macau"                        ,"MO"		, "853"     , 8     , "00"  },
	{"Macedonia"                    ,"MK"		, "389"     , 8     , "00"	},
	{"Madagascar"                   ,"MG"		, "261"     , 9     , "00"  },
	{"Malawi"                       ,"MW"		, "265"     , 9		, "00"  },
	{"Malaysia"                     ,"MY"		, "60"      , 9		, "00"	},
	{"Maldives"                     ,"MV"		, "960"     , 7	    , "00"  },
	{"Mali"                         ,"ML"		, "223"     , 8     , "00"  },
	{"Malta"                        ,"MT"       , "356"     , 8		, "00"  },
	{"Marshall Islands"				,"MH"		, "692"     , 7     , "011"	},
	{"Martinique"                   ,"MQ"		, "596"     , 9     , "00"  },
	{"Mauritania"                   ,"MR"		, "222"     , 8     , "00"  },
	{"Mauritius"                    ,"MU"		, "230"     , 7     , "00"	},
	{"Mayotte Island"               ,"YT"		, "262"     , 9     , "00"	},
	{"Mexico"                       ,"MX"		, "52"      , 10	, "00"	},
	{"Micronesia"                   ,"FM"		, "691"     , 7		, "011" },
	{"Moldova"                      ,"MD"		, "373"     , 8		, "00"  },
	{"Monaco"                       ,"MC"		, "377"     , 8     , "00"	},
	{"Mongolia"                     ,"MN"		, "976"     , 8     , "001" },
	{"Montenegro"                   ,"ME"		, "382"     , 8		, "00"  },
	{"Montserrat"                   ,"MS"		, "664"     , 10	, "011" },
	{"Morocco"                      ,"MA"		, "212"     , 9     , "00"	},
	{"Mozambique"                   ,"MZ"		, "258"     , 9     , "00"  },
	{"Myanmar"                      ,"MM"		, "95"      , 8		, "00"  },
	{"Namibia"                      ,"NA"		, "264"     , 9		, "00"	},
	{"Nauru"                        ,"NR"		, "674"     , 7	    , "00"  },
	{"Nepal"                        ,"NP"		, "43"      , 10	, "00"  },
	{"Netherlands"                  ,"NL"       , "31"      , 9		, "00"  },
	{"New Caledonia"				,"NC"		, "687"     , 6     , "00"	},
	{"New Zealand"                  ,"NZ"		, "64"      , 10	, "00"  },
	{"Nicaragua"                    ,"NI"		, "505"     , 8     , "00"  },
	{"Niger"                        ,"NE"		, "227"     , 8     , "00"	},
	{"Nigeria"                      ,"NG"		, "234"     , 10	, "009"	},
	{"Niue"                         ,"NU"		, "683"     , 4		, "00"	},
	{"Norfolk Island"	            ,"NF"		, "672"     , 5		, "00"  },
	{"Northern Mariana Islands"	    ,"MP"		, "1"       , 10	, "011" },
	{"Norway"                       ,"NO"		, "47"      , 8     , "00"	},
	{"Oman"                         ,"OM"		, "968"     , 8		, "00"  },
	{"Pakistan"                     ,"PK"		, "92"      , 10	, "00"  },
	{"Palau"                        ,"PW"		, "680"     , 7     , "011" },
	{"Palestine"                    ,"PS"		, "970"     , 9     , "00"	},
	{"Panama"                       ,"PA"		, "507"     , 8     , "00"  },
	{"Papua New Guinea"	            ,"PG"		, "675"     , 8		, "00"  },
	{"Paraguay"                     ,"PY"		, "595"     , 9		, "00"	},
	{"Peru"                         ,"PE"		, "51"      , 9	    , "00"  },
	{"Philippines"                  ,"PH"		, "63"      , 10	, "00"  },
	{"Poland"                       ,"PL"       , "48"      , 9		, "00"  },
	{"Portugal"                     ,"PT"		, "351"     , 9     , "00"	},
	{"Puerto Rico"                  ,"PR"		, "1"       , 10	, "011" },
	{"Qatar"                        ,"QA"		, "974"     , 8     , "00"  },
	{"R�union Island"				,"RE"		, "262"     , 9     , "011"	},
	{"Romania"                      ,"RO"		, "40"      , 9     , "00"	},
	{"Russian Federation"           ,"RU"		, "7"       , 10	, "8"	},
	{"Rwanda"                       ,"RW"		, "250"     , 9		, "00"  },
	{"Saint Helena"                 ,"SH"		, "290"     , 4		, "00"  },
	{"Saint Kitts and Nevis"		,"KN"		, "1"       , 10	, "011"	},
	{"Saint Lucia"                  ,"LC"		, "1"       , 10	, "011" },
	{"Saint Pierre and Miquelon"    ,"PM"		, "508"     , 6		, "00"  },
	{"Saint Vincent and the Grenadines","VC"	, "1"       , 10	, "011" },
	{"Samoa"                        ,"WS"		, "685"     , 7     , "0"	},
	{"San Marino"                   ,"SM"		, "378"     , 10	, "00"  },
	{"Sao Tome and Principe"        ,"ST"		, "239"     , 7		, "00"  },
	{"Saudi Arabia"                 ,"SA"		, "966"     , 9		, "00"	},
	{"Senegal"                      ,"SN"		, "221"     , 9	    , "00"  },
	{"Serbia"                       ,"RS"		, "381"     , 9     , "00"  },
	{"Seychelles"                   ,"SC"       , "248"     , 7		, "00"  },
	{"Sierra Leone"                 ,"SL"		, "232"     , 8     , "00"	},
	{"Singapore"                    ,"SG"		, "65"      , 8     , "001" },
	{"Slovakia"                     ,"SK"		, "421"     , 9     , "00"  },
	{"Slovenia"                     ,"SI"		, "386"     , 8     , "00"	},
	{"Solomon Islands"              ,"SB"		, "677"     , 7     , "00"	},
	{"Somalia"                      ,"SO"		, "252"     , 8		, "00"	},
	{"South Africa"                 ,"ZA"		, "27"      , 9		, "00"  },
	{"Spain"                        ,"ES"		, "34"      , 9		, "00"  },
	{"Sri Lanka"                    ,"LK"		, "94"      , 9     , "00"	},
	{"Sudan"                        ,"SD"		, "249"     , 9		, "00"  },
	{"Suriname"                     ,"SR"		, "597"     , 7		, "00"  },
	{"Swaziland"                    ,"SZ"		, "268"     , 8     , "00"  },
	{"Sweden"                       ,"SE"		, "1"       , 9     , "00"	},
	{"Switzerland"                  ,"XK"		, "41"      , 9		, "00"	},
	{"Syria"                        ,"SY"		, "963"     , 9		, "00"  },
	{"Taiwan"                       ,"TW"		, "886"     , 9		, "810"	},
	{"Tajikistan"                   ,"TJ"		, "992"     , 9	    , "002" },
	{"Tanzania"                     ,"TZ"		, "255"     , 9     , "000" },
	{"Thailand"                     ,"TH"       , "66"      , 9		, "001" },
	{"Togo"                         ,"TG"		, "228"     , 8     , "00"	},
	{"Tokelau"                      ,"TK"		, "690"     , 4     , "00"  },
	{"Tonga"                        ,"TO"		, "676"     , 5     , "00"  },
	{"Trinidad and Tobago"			,"TT"		, "1"       , 10    , "011"	},
	{"Tunisia"                      ,"TN"		, "216"     , 8     , "00"	},
	{"Turkey"                       ,"TR"		, "90"      , 10	, "00"	},
	{"Turkmenistan"                 ,"TM"		, "993"     , 8		, "00"  },
	{"Turks and Caicos Islands"	    ,"TC"		, "1"       , 7		, "0"   },
	{"Tuvalu"                       ,"TV"		, "688"     , 5     , "00"	},
	{"Uganda"                       ,"UG"		, "256"     , 9     , "000" },
	{"Ukraine"                      ,"UA"		, "380"     , 9		, "00"  },
	{"United Arab Emirates"	        ,"AE"		, "971"     , 9     , "00"  },
	{"United Kingdom"               ,"GB"		, "44"      , 10	, "00"	},
/*	{"United Kingdom"               ,"UK"		, "44"      , 10	, "00"	},*/
	{"United States"                ,"US"		, "1"       , 10	, "011" },
	{"Uruguay"                      ,"UY"		, "598"     , 8		, "00"  },
	{"Uzbekistan"                   ,"UZ"		, "998"     , 9		, "8"	},
	{"Vanuatu"                      ,"VU"		, "678"     , 7	    , "00"  },
	{"Venezuela"                    ,"VE"		, "58"      , 10	, "00"  },
	{"Vietnam"                      ,"VN"		, "84"      , 9     , "00"  },
	{"Wallis and Futuna"	        ,"WF"		, "681"     , 5		, "00"  },
	{"Yemen"                        ,"YE"		, "967"     , 9     , "00"  },
	{"Zambia"                       ,"ZM"		, "260"     , 9     , "00"	},
	{"Zimbabwe"                     ,"ZW"		, "263"     , 9     , "00"  },
	{NULL                           ,NULL       ,  ""       , 0     , NULL	}
};
static LinphoneDialPlan most_common_dialplan={ "generic" ,"", "", 10, "00"};

int linphone_dial_plan_lookup_ccc_from_e164(const char* e164) {
	LinphoneDialPlan* dial_plan;
	LinphoneDialPlan* elected_dial_plan=NULL;
	unsigned int found;
	unsigned int i=0;
	
	if (e164[0]!='+') {
		return -1;/*not an e164 number*/
	}
	if (e164[1]=='1') {
		/*USA case*/
		return 1;
	}
	do {
		found=0;
		i++;
		for (dial_plan=(LinphoneDialPlan*)dial_plans; dial_plan->country!=NULL; dial_plan++) {
			if (strncmp(dial_plan->ccc,&e164[1],i) == 0) {
				elected_dial_plan=dial_plan;
				found++;
			}
		}
	} while ((found>1 || found==0) && i < sizeof(dial_plan->ccc));
	if (found==1) {
		return atoi(elected_dial_plan->ccc);
	} else {
		return -1; /*not found */
	}

}
int linphone_dial_plan_lookup_ccc_from_iso(const char* iso) {
	LinphoneDialPlan* dial_plan;
	for (dial_plan=(LinphoneDialPlan*)dial_plans; dial_plan->country!=NULL; dial_plan++) {
		if (strcmp(iso, dial_plan->iso_country_code)==0) {
			return atoi(dial_plan->ccc);
		}
	}
	return -1;
}

const LinphoneDialPlan* linphone_dial_plan_by_ccc_as_int(int ccc) {
	int i;
	char ccc_as_char[16] = {0};
	snprintf(ccc_as_char,sizeof(ccc_as_char)-1,"%i",ccc);
	
	for(i=0;dial_plans[i].country!=NULL;++i){
		if (strcmp(ccc_as_char,dial_plans[i].ccc)==0){
			return &dial_plans[i];
		}
	}
	/*else return a generic "most common" dial plan*/
	return &most_common_dialplan;
}


const LinphoneDialPlan* linphone_dial_plan_by_ccc(const char *ccc) {
	if (!ccc) {
		return &most_common_dialplan;
	}

	return linphone_dial_plan_by_ccc_as_int((int)strtol(ccc,NULL,10));
}

const LinphoneDialPlan* linphone_dial_plan_get_all() {
	return dial_plans;
}

bool_t linphone_dial_plan_is_generic(const LinphoneDialPlan *ccc) {
	if (strcmp(ccc->country, most_common_dialplan.country) == 0)
		return TRUE;
	return FALSE;
}
