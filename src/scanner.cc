#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>

#include "scanner.h"

/***********************************************************/
/************** Private methods of scanner_t ***************/
/***********************************************************/
void scanner_t::skipspaces(void)
{
    while (!eofile)
    {
        if (!isspace(ch) && (int)ch != 10)
            break;
        eofile = (inf.get(ch) == 0);
        incrementPosition();
    }
}

/* Called when '/' is found */
bool scanner_t::skipcomment(void)
{
    /* Save state of scanner in case we're not actually processing a
     * comment and need to backtrack */
    char chstart = ch;
    unsigned int linestart = line, colstart = col;
    bool eofilestart = eofile;
    streampos pos = inf.tellg();

    /* Setup for searching for the comment */
    string str = "";
    bool prefixFound = false;
    bool suffixFound = false;
    string prefix("/*");
    string suffix("*/");
    
    /* Start searching */
    while (!eofile && (!prefixFound || !suffixFound))
    {
        str += ch;          // save comment so we can search for key characters
        if (!prefixFound)
        {
            /* Look for prefix */
            if (str.compare(0,prefix.size(),prefix) == 0)
                prefixFound = true;
        }
        if (!prefixFound && str.size() >= 2)
            break;          // break if the string didn't start with '/*' afterall
        if (prefixFound && !suffixFound)
        {
            /* Look for suffix */
            if (str.compare(str.size()-2,str.size(),suffix) == 0)
                suffixFound = true;
        }
        eofile = (inf.get(ch) == 0);
        incrementPosition();
    }
    /* No comment found */
    if (!prefixFound)
    {
        /* Return scanner to state before skipcomment was called */
        ch = chstart; line = linestart; col = colstart; eofile = eofilestart;
        inf.seekg(pos);
        return false;
    }
    /* Comment found, return true */
    else
        return true;        // prefixFound && !suffixFound => whole file is comment after prefix
}

void scanner_t::getnumber(int &number)
{
    string num = "";
    while (!eofile)
    {
        if (isdigit(ch))
            num += ch;                      // build up the number as a string
        else
        {
            number = atoi(num.c_str());     // convert string to int at end
            break;
        }
        eofile = (inf.get(ch) == 0);
        incrementPosition();
    }
}


name_t scanner_t::getname(namestring_t &str)
{
    namestring_t outstr = "";
    string toolongstr = "";
    bool error = false;

    while (!eofile)
    {
        if (isalnum(ch))
        {
            if (outstr.length() == maxlength)
            {
                /* Exceeded maxlength */
                error = true;
                toolongstr += ch;
            }
            else
            {
                /* Normal operation before maxlength reached */
                outstr += ch;
                toolongstr += ch;
            }
        }
        else
        {
            /* end of name string when no longer alpha characters */
            str = outstr;
            if (error)
                cout << "Warning: " << toolongstr << " exceeded maxlength " << maxlength << endl;
            return namesObj->lookup(outstr);    // add name to names class
        }
        eofile = (inf.get(ch) == 0);
        incrementPosition();
    }
    return namesObj->lookup(outstr);    // still add name to names class if eofile reached
}

void scanner_t::getpunc(namestring_t &str)
{
    str = "";
    
    while (!eofile)
    {
        if (!isalnum(ch) && !isspace(ch))
            str += ch;
        else
            return;
        eofile = (inf.get(ch) == 0);
        incrementPosition();
    }
}

/* Equate special strings in the definition file to a symbol_t
 * Only called for non-numbers */
symbol_t scanner_t::symbolType(namestring_t namestring)
{
    symbol_t s;
    char firstchar = namestring[0];

    /* namestring.compare(string) returns 0 if namestring and string identical */
    if      (!namestring.compare("STARTFILE"))   s = startfsym;
    else if (!namestring.compare("DEVICES"))     s = devsym;
    else if (!namestring.compare("CONNECTIONS")) s = connsym;
    else if (!namestring.compare("MONITORS"))    s = monsym;
    else if (!namestring.compare("END"))         s = endsym;
    else if (!namestring.compare("ENDFILE"))     s = endfsym;
    else if (!namestring.compare("SWITCH"))      s = switchsym;
    else if (!namestring.compare("AND"))         s = andsym;
    else if (!namestring.compare("NAND"))        s = nandsym;
    else if (!namestring.compare("OR"))          s = orsym;
    else if (!namestring.compare("NOR"))         s = norsym;
    else if (!namestring.compare("DTYPE"))       s = dtypesym;
    else if (!namestring.compare("XOR"))         s = xorsym;
    else if (!namestring.compare(","))           s = commasym;
    else if (!namestring.compare(";"))           s = semicolsym;
    else if (!namestring.compare("("))           s = opsym;
    else if (!namestring.compare(")"))           s = cpsym;
    else if (!namestring.compare("="))           s = equalsym;
    else if (!namestring.compare("."))           s = dotsym;
    /* If first char is alphabetic then namestring was retrieved with getname and is a name */
    else if (isalpha(firstchar))                 s = strsym;
    else if (!namestring.compare("\0"))          s = eofsym;
    else                                         s = badsym;

    return s;
}

void scanner_t::incrementPosition(void)
{
    col++;
    if (ch == '\n')
    {
        line++;
        col = 0;
    }
}

punc_t scanner_t::puncType(char ch)
{
    if ((ch == ',') || (ch == ';') || (ch == '(') || (ch == ')') || (ch == '=') || (ch == '.'))
        return validPunc;
    else if (ch == '/')
        return slash;
    else if (ch == '*')
        return star;
    else
        return invalidPunc;
}







/***********************************************************/
/************** Public methods of scanner_t ****************/
/***********************************************************/
scanner_t::scanner_t(names *namesObjin, const char *defname)
{
    namesObj = namesObjin;
    inf.open(defname);
    if (!inf)
    {
        cout << "Error: cannot open file " << defname << "for reading" << endl;
        exit(1);
    }
    line = 1;   // first line is line 1
    col = 1;    // First possible cursor position is column 1
    
    eofile = (inf.get(ch) == 0);    // get first character
    incrementPosition();
}

scanner_t::~scanner_t()
{
    return;
}

void scanner_t::nextSymbol(symbol_t &symbol, namestring_t &namestring, int &num)
{
    if (!eofile)
    {
        /* Skip spaces first */
        skipspaces();

        /* Then check if the next symbol is a number */
        if (isdigit(ch))
        {
            //cout << "NUMBER" << endl;
            getnumber(num);
            symbol = numsym;
            return;
        }

        /* Process symbols that are some sort of string */
        else if (isalpha(ch))
        {
            getname(namestring);
            symbol = symbolType(namestring);
            return;
        }

        /* If the character is a slash, guess that it's going to be a comment */
        else if (puncType(ch) == slash)
        {
            /* skipcomment discards everything in between comment
             * characters and returns the input file to the place it
             * started if it turns out the '/' character wasn't followed
             * by '*' */
            bool isComment = skipcomment();
            if (!isComment)
            {
                /* Turns out it was some sort of string starting with '/' */
                getpunc(namestring);
                symbol = symbolType(namestring);
                return;
            }
            else
            {
                /* Comment has been discarded, now call nextSymbol so
                 * that the variables passed in actually get populated */
                nextSymbol(symbol, namestring, num);
            }
        }

        else
        {
            /* Some sort of string starting with punctuation */
            getpunc(namestring);
            symbol = symbolType(namestring);
            /* ch will still be \n if eofile is reached so overwrite it
             * if the end of the file has been read in already */
            if (eofile)
                symbol = eofsym;
            return;
        }
    }

    else
    {
        cout << "EOF" << endl;
        symbol = eofsym;
        return;
    }
}

void scanner_t::getPosition(int &oLine, int &oCol, bool &ok)
{
    ok = true;

    if (line >= 0) oLine = line;
    else ok = false;

    if (col >= 0) oCol = col-1;     // column has moved on 1 since the last symbol outputted
    else ok = false;
}
