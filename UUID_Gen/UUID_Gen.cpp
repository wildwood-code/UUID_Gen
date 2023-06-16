// UUID_Gen - Universally Unique IDentifier command-line generator
//
// Generates a unique 128-bit number coded as a hexadecimal string based upon
// version-4, variant "10" DCE 1.1, ISO/IEC 11578:1996 or
// version-4, variant "110" Microsoft GUID
//
// Examples:
//   UUID mode:
//     F4D5B5AD-6D32-4B15-B303-7638173696FD
//     7B960408-E65C-4943-99E2-C92590BCAA15
//   GUID mode:
//     {D8115A02-7C7B-4422-DE55-52534D00BDB9}
//     {983E505F-68E2-4FE0-C0C7-911AFDE2E51D}

#include <iostream>
#include <random>
#include <ctime>
#include <regex>
#include "Windows.h"

using namespace std;


/// @brief Generate UUID pattern
/// @param gen      : reference to mt19937_64 random number generator
/// @param is_guid  : false->UUID form {default}, true->GUID form
/// @param is_lower : false->uppercase hex {default}, true->lowercase hex
/// @return string representation of UUID/GUID
string gen_uuid(mt19937_64 &gen, bool is_guid=false, bool is_lower=false)
{
    uniform_int_distribution<uint16_t> dis16;
    char szbuf[40];
    uint16_t uuid[8];

    // generate the random sequence
    for (auto i = 0; i < 8; ++i)
        uuid[i] = dis16(gen);

    // set the version: version-4
    uuid[3] &= 0x0FFF;
    uuid[3] |= 0x4000;

    // set the variant
    if (is_guid)
    {   // UUID version 4, variant "110" => Microsoft GUID
        uuid[4] &= 0x1FFF;
        uuid[4] |= 0xC000;
    }
    else
    {   // UUID version 4, variant "10" => DCE 1.1, ISO/IEC 11578:1996
        uuid[4] &= 0x3FFF;
        uuid[4] |= 0x8000;
    }

    // format as a hexadecimal string
    if (is_lower)
        sprintf_s(szbuf, "%04x%04x-%04x-%04x-%04x-%04x%04x%04x", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7]);
    else
        sprintf_s(szbuf, "%04X%04X-%04X-%04X-%04X-%04X%04X%04X", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7]);

    return string(szbuf);
}


/// @brief function to copy string to clipboard source: http://www.cplusplus.com/forum/beginner/14349/ 
///     with bug fix to copy final character
/// @param s     : string to copy to clipboard
void toClipboard(const std::string& s)
{
    OpenClipboard(0);
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, s.size()+1);
    if (!hg)
    {
        CloseClipboard();
        return;
    }
    LPVOID pgl = GlobalLock(hg);
    if (pgl != NULL)
    {
        memcpy(pgl, s.c_str(), s.size() + 1);
        GlobalUnlock(hg);
        SetClipboardData(CF_TEXT, hg);
    }
    
    CloseClipboard();
    GlobalFree(hg);
}


// main entry point
int main(int argc, char *argv[])
{
    int retval = 0;

    // parameters set on the command-line (load defaults here)
    bool is_guid = false;
    bool is_lower = false;
    bool is_help = false;
    bool is_error = false;
    bool is_clipboard = false;
    bool is_quiet = false;
    bool is_delim = false;
    int genNumber = 1;
    string strLead("");
    string strTail("");

    // regular expressions
    const regex reClip("^(-|\\/)CL?I?P?$", regex::icase);
    const regex reQuiet("^(-|\\/)QU?I?E?T?$", regex::icase);
    const regex reLower("^(-|\\/)L(C|OW?E?R?)?$", regex::icase);
    const regex reUpper("^(-|\\/)U(C|PP?E?R?)?$", regex::icase);
    const regex reGUID("^(-|\\/)GU?I?D?$", regex::icase);
    const regex reHelp("^(-|\\/)(\\?|HE?L?P?|USA?G?E?)$", regex::icase);
    const regex reDelim1("^(?:-|\\/)DE?L?I?M?(?:=|\\:)(.*)$", regex::icase);
    const regex reDelim2("^(-|\\/)DE?L?I?M?$", regex::icase);
    const regex reDelimCS("^(.+),(.+)$");
    const regex reGenNumber("^(?:-|\\/)NU?M?B?E?R?(?:=|\\:)(.*)$", regex::icase);
    smatch smMatch;

    for (auto a = 1; a < argc; ++a)
    {   // parse each command-line argument
        string str = argv[a];

        if (regex_match(str, reClip))
        {   // copy UUID to clipboard
            is_clipboard = true;
        }
        else if (regex_match(str, reQuiet))
        {   // quiet mode, not output, automatically copy to clipboard
            is_quiet = true;
            is_clipboard = true;
        }
        else if (regex_match(str, reLower))
        {   // force lower case
            is_lower = true;
        }
        else if (regex_match(str, reUpper))
        {   // force upper case
            is_lower = false;
        }
        else if (regex_match(str, reGUID))
        {   // Microsoft GUID mode
            is_guid = true;
        }
        else if (regex_match(str, reHelp))
        {   // generate some help and exit
            is_help = true;
        }
        else if (regex_match(str, smMatch, reDelim1))
        {   // delimiter specified
            string strDelim = smMatch[1];
            size_t nDelim = strDelim.length();
            is_delim = true;

            if (nDelim == 0)
            {   // delimiter disabled (empty)
                strLead = "";
                strTail = "";
            }
            else if (nDelim == 1)
            {   // single char, use for both
                strLead = strDelim;
                strTail = strDelim;
            }
            else if (regex_match(strDelim, smMatch, reDelimCS))
            {   // comma-separated notation
                strLead = smMatch[1];
                strTail = smMatch[2];
            }
            else
            {   // just take it half-and-half
                strLead = strDelim.substr(0, nDelim / 2);
                strTail = strDelim.substr(nDelim / 2, string::npos);
            }
        }
        else if (regex_match(str, reDelim2)) {   // delimiter disabled - this can disable the auto-delimiters for GUID format
            is_delim = true;
            strLead = "";
            strTail = "";
        }
        else if (regex_match(str, smMatch, reGenNumber)) {   // -Number=## specified
            string strDelim = smMatch[1];
            size_t nDelim = strDelim.length();

            if (nDelim == 0) {   // delimiter disabled (empty)
                genNumber = 1;  // still create 1 even if they can't form it correctly
            } else {   // parse the number
                genNumber = stoi(strDelim);
            }
        } 
        else
        {   // something unrecognized... generate an error
            cerr << "Invalid command-line argument: \"" << str << "\"" << endl;
            is_error = true;
        }
    }

    if (is_guid && !is_delim)
    {   // add standard GUID delimiters if not overridden
        strLead = "{";
        strTail = "}";
    }

    if (is_error)
    {   // oops... something went wrong
        retval = 1;
    }
    else if (is_help)
    {   // print usage to stderr
        cerr << "\n" << argv[0] << " -? -g -l -u -c -q -d=delim -n=#\n"
             << "  -?   help\n"
             << "  -g   GUID format (surround with curly braces)\n"
             << "  -l   lower-case hex\n"
             << "  -u   upper-case hex (default)\n"
             << "  -c   copy UUID to clipboard\n"
             << "  -q   quiet mode (just copy to clipboard, no output)\n"
             << "  -d   disable GUID curly braces\n"
             << "  -d=delim\n"
             << "       delim=  to disable GUID curly braces\n"
             << "       delim=x for single char x on both ends\n"
             << "       delim=xy for x at head and y at tail\n"
             << "       delim=xxx,yyy for xxx at head and y at tail\n"
             << "  -n=#\n"
             << "       Generate a number of GUIDs, one to a line\n"
             << "\n"
             << "  Version 1.2, 2023-06-16\n"
             << "    KSM minor tweak to help\n"
             << "  Version 1.1, 2022-09-08\n"
             << "    David Smart, David@Smart-Family.net, added -n=# command, tweaked the help, shortened the name.\n"
             << "  Version 1.0, 2020-08-04\n"
             << "    Kerry S. Martin, martin@wild-wood.net, free for commercial and non-commercial use\n"
             << endl;
    }
    else
    {
        // generate the UUID
        mt19937_64 gen;
        gen.seed((mt19937_64::result_type)time(NULL));
        string returnString = "";
        while (genNumber) {
            string uuid = strLead + gen_uuid(gen, is_guid, is_lower) + strTail;
            returnString += uuid;
            if (--genNumber)
                returnString += "\n";
        }
        if (!is_quiet) {   // output the UUID to stdout
            cout << returnString;
        }
        if (is_clipboard)
        {   // copy it to the clipboard
            toClipboard(returnString);
        }
    }
    return retval;
}


// Kerry S. Martin, martin@wild-wood.net, 2020-08-04, 2023-06-16
// David Smart, David@Smart-Family.net, 2022-09-08
