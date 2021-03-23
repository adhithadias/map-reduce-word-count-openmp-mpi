
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/**
 * Format string with only lower case alphabetic letters
 */
char * format_string(char *original) {
    int len = strlen(original);
    char *word = (char *) malloc(len*sizeof(char));
    int c = 0;
    int i;
    for(i=0;i<len;i++)
    {
        if(isalnum(original[i]) || original[i]=='\'')
        {
            word[c]=tolower(original[i]);
            c++;
        }
    }
    word[c]='\0';
    return word;
}