#ifndef scanner_h
#define scanner_h

#include <fstream>
#include <string>

#include "typedefs.h"
#include "names.h"

typedef enum {validPunc, invalidPunc, slash, star} punc_t;

class scanner_t
{
private:
    ifstream inf;           // input file
    names *namesObj;        // names object
    char ch;                // current character
    bool eofile;            // true when end of file is reached
    unsigned int line, col; // current line and column numbers
    
    void skipspaces(void);
      /* Skip spaces; will usually end on \n at eof */

    void getnumber(symbol_t &symbol);
      /* Process a string that starts with a digit */

    name_t getname(symbol_t &symbol);
      /* Process a string that starts with an alpha character */

    void getpunc(symbol_t &symbol);
      /* Process a string that doesn't start with space, alpha or digit */

    symboltype_t symbolType(namestring_t namestring);
      /* Return the symbol_t for a particular string such as 'END' or '.' */

    void incrementPosition(void);
      /* Track line and column number */

    punc_t puncType(char ch);
      /* Return the type of punctuation, and invalidPunc if the
       * punctuation shouldn't appear in a definition file */

    bool skipcomment(void);
      /* Process a string (hopefully a comment) that starts with a '/'
       * character */
    
    void saveCurPosition(symbol_t &symbol);
      /* Save the current line and col values into symbol and subtract
       * 1 from col to make it correct */

public:
    scanner_t(names *namesObjin, const char *defname);
      /* Constructor for the scanner class
       * Arg1: pointer to an instance of names class
       * Arg2: pointer to the definition file */

    ~scanner_t();
      /* Destructor for the scanner class */

    void nextSymbol(symbol_t &symbol);
      /* Return the next symbol in the definition file in a struct */

};

#endif  /* scanner_h */
