#include <string>
#include <vector>
#include <algorithm>

using namespace std;

struct keyValuePairs {
	vector<string> keys;
	vector<string> values;
};
typedef struct keyValuePairs keyValuePairs;
int keyValues(const char* STR, keyValuePairs *kvp, const char *valDelim,const char* pairDelim);
