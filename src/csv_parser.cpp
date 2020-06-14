#include "csv_parser.h"

CSV_Parser::CSV_Parser(const char * s, const char * fmt, bool has_header_, char delimiter_, char quote_char_) :
  cols_count(strlen(fmt)),
  types(strdup(fmt)),
  has_header(has_header_),
  delimiter(delimiter_),
  quote_char(quote_char_)
{
  const char delim_chars[4] = {'\r', '\n', delimiter, 0}; 
  rows_count = CountRows(s);                                  // can't be in initializer list because it relies on other members
  keys =   (char**)calloc(cols_count, sizeof(char*));         // calloc fills memory with 0's so then I can simply use "if(keys[i]) { do something with key[i] }"
  values = (void**)calloc(cols_count, sizeof(void*));

  const char * base_s = s;
  for (int col = 0; col < strlen(fmt); col++) {
      int key_len = 0;
      keys[col] = ParseStringValue(s, delim_chars, &key_len);
      if (fmt[col] == '-' || !has_header) {
        free(keys[col]);
        keys[col] = 0; 
      } 
      if (fmt[col] != '-')
        values[col] = malloc(GetTypeSize(fmt[col]) * rows_count);
        
      s += key_len + 1; 
      while(*s == '\n' || *s == '\r') s++;
  }    

  if (!has_header) 
    s = base_s;

  for (int row = 0; row < rows_count; row++) {
    for (int col = 0; col < strlen(fmt); col++) {
      int val_len = 0;
      char * val;
      if (fmt[col] != 's') { 
        val_len = strcspn(s, delim_chars);
        val = strndup(s, val_len);
        RemoveEnclosingDoubleQuotes(val);
      } else {
        val = ParseStringValue(s, delim_chars, &val_len);
      }

      SaveNewValue(val, fmt[col], row, col);

      s += val_len + 1;
      free(val);
      while(*s == '\n' || *s == '\r') s++;
    }
  }
}

CSV_Parser::~CSV_Parser() {
  for (int col = 0; col < cols_count; col++) {
    if (types[col] == 's')
      for (int row = 0; row < rows_count; row++)
        free(((char**)values[col])[row]);
    
    if (keys[col])   free(keys[col]);
    if (values[col]) free(values[col]);
  }
  free(keys);
  free(values);
  free(types);
}

/*          
  This function is not meant to properly parse string values enclosed in double quotes.
  This function is meant to be used for numeric values that happen to be enclosed in double quotes.
  It assumes that the only possible double quotes are at the begining and end.
  Example:
    
    my_ints,my_floats\n
    "1","3.3"\n
    "2","6.6"
           */
void CSV_Parser::RemoveEnclosingDoubleQuotes(char * s) {
  if (*s  == quote_char) {
    // shift whole string to left
    while(*++s) *(s-1) = *s;

    // remove last char if it's double quotes
    if (*(s-2) == quote_char) *(s-2) = 0;
  }

  // if the string didn't start with double quotes then assume it doesn't end with double quotes
}


char * CSV_Parser::ParseStringValue(const char * s, const char * delim_chars, int * chars_occupied) {
  
  // value is not enclosed in double quotes
  if(*s != quote_char) {
    *chars_occupied = strcspn(s, delim_chars);
    return strndup(s, *chars_occupied);
  }

  // value is enclosed in double quotes
  // being enclosed in double quotes automatically means that the total number of occupied characters is 2 more than usual
  *chars_occupied = 2;
  
  s += 1;
  const char * base = s;

  int len = 0; 
  while(*s) {
    if (*s == quote_char && *(s+1) != quote_char) {
      // end of string was found 
      break;
    }
    byte to_add = (*s == quote_char && *(s+1) == quote_char) ? 2 : 1;
    s += to_add;
    *chars_occupied += to_add;
    
    len += 1;
  }

  // copy string and turn 2's of adjacent double quotes into 1's
  char * new_s = (char*)malloc(len + 1);
  new_s[len] = 0;
  char * base_new_s = new_s;
  
  s = base;
  for (int i = 0; i < len; i++) {
    *new_s++ = *s++;
    if (*(s-1) == quote_char && *s == quote_char)
      s += 1;
  }
  return base_new_s;
}


int CSV_Parser::CountRows(const char *s) {
  int count = 0;
  const char delim_chars[4] = {'\r', '\n', delimiter, 0};
  while(*s) {
    for (int col = 0; col < cols_count; col++) {
      int len = 0;
      if (types[col] == 's' || (has_header && count == 0)) 
        free(ParseStringValue(s, delim_chars, &len));
      else 
        len = strcspn(s, delim_chars);
      
      s += len + 1; 
      while(*s == '\n' || *s == '\r') 
        s++; 
    }   
    count += 1;
    
    if (*(s-1) == 0)
      break;
  }
  return count - has_header;
}


int8_t CSV_Parser::GetTypeSize(char type_specifier) {
  switch (type_specifier) {
    case 's': return sizeof(char*);   // c-like string
    case 'f': return sizeof(float); 
    case 'L': return sizeof(int32_t); // 32-bit signed number (not higher than 2147483647)       
    case 'd': return sizeof(int16_t); // 16-bit signed number (not higher than 32767)
    case 'c': return sizeof(char);    // 8-bit signed number  (not higher than 127)
    case 'x': return sizeof(int32_t); // hex input is stored as long (32-bit signed number)
    case '-': return 0;   
    case   0: return 0;
    default : Serial.println("CSV_Parser, wrong fmt specifier = " + String(type_specifier));
  }
  return 0;
}

const char * CSV_Parser::GetTypeName(char type_specifier) {
  switch(type_specifier){
      case 's': return "char*";          
      case 'f': return "float";
      case 'L': return "int32_t";
      case 'd': return "int16_t";
      case 'c': return "char";
      case 'x': return "hex (int32_t)"; // hex input, but it's stored as int32_t
      case '-': return "unused";
      case   0: return "unused";
      default : return "unknown";
  }
}

void CSV_Parser::SaveNewValue(const char * val, char type_specifier, int row, int col) {
  switch (type_specifier) {
    case 's': { ((char**)  values[col])[row] = strdup(val);                 break; } // c-like string
    case 'f': { ((float*)  values[col])[row] = (float)atof(val);            break; }
    case 'L': { ((int32_t*)values[col])[row] = (int32_t)atol(val);          break; } // 32-bit signed number (not higher than 2147483647) 
    case 'd': { ((int16_t*)values[col])[row] = (int16_t)atoi(val);          break; } // 16-bit signed number (not higher than 32767)
    case 'c': { ((char*)   values[col])[row] = (char)atoi(val);             break; } // 8-bit signed number  (not higher than 127)
    case 'x': { ((int32_t*)values[col])[row] = (int32_t)strtol(val, 0, 16); break; } // hex input is stored as long (32-bit signed number)
    case '-': break;
  }
}


void CSV_Parser::PrintKeys() {
  Serial.println("Keys:");
  for (int col = 0; col < cols_count; col++) 
    Serial.println(String(col) + ". Key = " + String(keys[col] ? keys[col] : "unused"));
}

int CSV_Parser::GetColumnsCount() { return cols_count; }
int CSV_Parser::GetRowsCount() { return rows_count; } // excluding header

void * CSV_Parser::GetValues(const char * key) {
  for (int col = 0; col < cols_count; col++) 
    if (keys[col] && !strcmp(keys[col], key))
      return values[col];
  return (void*)0;
}

void * CSV_Parser::GetValues(int index)          { return index < cols_count ? values[index] : (void*)0; }
void * CSV_Parser::operator [] (const char *key) { return GetValues(key);   }
void * CSV_Parser::operator [] (int index)       { return GetValues(index); }

CSV_Parser::operator String() {
  String ret = "CSV_Parser:\n";
  ret += "  header fields:\n";
  for (int col = 0; col < cols_count; col++) 
    ret += "    " + String(keys[col]) + " (" + GetTypeName(types[col]) + ")\n"; 

  ret += "  rows number = " + String(rows_count);
  return ret;
}

void CSV_Parser::Print() {
  Serial.println("CSV_Parser content:");
  Serial.println("   Header:");
  Serial.print("      ");
  for (int col = 0; col < cols_count; col++) { 
    Serial.print(keys[col] ? keys[col] : "unused"); if(col == cols_count - 1) { continue; }
    Serial.print(" | ");
  }
  Serial.println();

  Serial.println("   Types:");
  Serial.print("      ");
  for (int col = 0; col < cols_count; col++) { 
    Serial.print(GetTypeName(types[col])); if(col == cols_count - 1) { continue; }
    Serial.print(" | ");
  }
  Serial.println();
  
  Serial.println("   Values:");
  for (int i = 0; i < rows_count; i++) {
    Serial.print("      ");
    for (int j = 0; j < cols_count; j++) {     
      switch(types[j]){
          case 's': Serial.print( ((char**)  values[j])[i] );       break;
          case 'f': Serial.print( ((float*)  values[j])[i] );       break;
          case 'L': Serial.print( ((int32_t*)values[j])[i]  , DEC); break;          
          case 'd': Serial.print( ((int16_t*)values[j])[i]  , DEC); break;
          case 'c': Serial.print( ((char*)   values[j])[i]  , DEC); break;
          case 'x': Serial.print( ((int32_t*)values[j])[i]  , HEX); break;
          case '-': Serial.print('-'); break;
          case   0: Serial.print('-'); break;
      }
      if(j == cols_count - 1) { continue; }
      Serial.print(" | ");
    }
    Serial.println();
  }
}
