#include "kvp.h"

int keyValues(const char* STR, keyValuePairs *kvp, const char *valDelim,const char* pairDelim)
{
	int err = 0;
    // Skip delimiters at beginning.
	string str(STR);
	string valDelimStr(valDelim);
	string pairDelimStr(pairDelim);
	
	string parse;
	string key;
	string value;
	
	std::vector<string>::iterator iter;
	
	vector<string> pairs;
	
    string::size_type lastPos = str.find_first_not_of(pairDelimStr, 0);
    string::size_type pos     = str.find_first_of(pairDelimStr, lastPos);
	
    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        pairs.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(pairDelimStr, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(pairDelimStr, lastPos);
    }
	
	
	while(pairs.size()){
		parse = pairs.at(0);	
		
		lastPos = parse.find_first_not_of(valDelimStr, 0);
		pos     = parse.find_first_of(valDelimStr, lastPos);
		
		if(lastPos<0 || lastPos>=parse.size()){
			return 1;
		}
		
		key = parse.substr(lastPos, pos - lastPos);
		
		if(key.size() ==0){
			return 1;
		} else {
			kvp->keys.push_back(key);
		}
				
		lastPos = parse.find_first_not_of(valDelimStr, pos);
        pos = parse.find_first_of(valDelimStr, lastPos);
		
		if(lastPos<0 || lastPos>=parse.size()){
			kvp->values.push_back("\0");
			iter = pairs.begin();
			pairs.erase(iter);
			continue;
		}
		
		value = parse.substr(lastPos,pos-lastPos);
		kvp->values.push_back(value);
				
		iter = pairs.begin();
		pairs.erase(iter);
	}
	
	
	return err;
}
