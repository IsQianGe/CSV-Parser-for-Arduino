# What is it
It's a class to which you can supply:  
- csv string (including new-line characters)  
- [format string](#how-to-specify-value-types) (where each letter specifies type of value for each column)  

Class parses that string, stores the values and provides you with:  
- easily accessible set of arrays (their types are specified by the format string)  


# Motivation
I wanted to parse [covid-19 csv](https://github.com/tomwhite/covid-19-uk-data) data and couldn't find any csv parser for Arduino. So instead of rushing with a quick/dirty solution, I decided to write something that could be reused in the future (possibly by other people too).  

# Usage example
```cpp
char * csv_str = "my_strings,my_longs,my_ints,my_chars,my_floats,my_hex,my_to_be_ignored\n"
		 "hello,70000,140,10,3.33,FF0000,this_value_wont_be_stored\n"
		 "world,80000,150,20,7.77,0000FF,this_value_wont_be_stored\n"
		 "noice,90000,160,30,9.99,FFFFFF,this_value_wont_be_stored";

CSV_Parser cp(csv_str, /*format*/ "sLdcfx-");

char  **strings =       (char**)cp["my_strings"];
long  *longs =          (long*) cp["my_longs"];
int   *ints =           (int*)  cp["my_ints"];
char  *chars =          (char*) cp["my_chars"];
float *floats =         (float*)cp["my_floats"];
long  *longs_from_hex = (long*) cp["my_hex"]; // CSV_Parser stores hex as longs (casting to int* would point to wrong address when ints[ind] is used)

for (int i = 0; i < cp.GetRowsCount(); i++) {
Serial.print(strings[i]);             Serial.print(" - ");
Serial.print(longs[i], DEC);          Serial.print(" - ");
Serial.print(ints[i], DEC);           Serial.print(" - ");
Serial.print(chars[i], DEC);          Serial.print(" - ");
Serial.print(floats[i]);              Serial.print(" - ");
Serial.print(longs_from_hex[i], HEX); Serial.println();
```

Output:  
> hello - 70000 - 140 - 10 - 3.33 - FF0000  
> world - 80000 - 150 - 20 - 7.77 - FF  
> noice - 90000 - 160 - 20 - 9.99 - FFFFFF   

Notice how each character within `"sLdcfx-"` string specifies different type for each column. It is very important to set this format right. 
We could set each solumn to be strings like "sssssss", however this would use more memory than it's really needed. If we wanted to store a large array of small numerical values (e.g. under 128), then using "c" specifier would be appropriate. See "How to specify value types" section for full list of available specifiers and their descriptions.  

# Things to consider  
CSV must:  
* be separated by comma  

Programmer must:  
* know and specify what type of values are stored in each of the CSV columns  
* cast returned values appropriately  
		  
# How to specify value types 
By supplying format parameter to constructor of CSV_Parser. Example:
```cpp
CSV_Parser cp("my_strings,my_floats\nhello,1.1\nworld,2.2", "sf");
```

Example above is specifying "s" (string) for the 1st column, and "f" (float) for the 2nd column. Possible specifiers are:  
L - long (32-bit signed value)  
f - float  
s - string (C-like string, not a "String" Arduino object, just a char pointer, terminated by 0)  
d - int (16-bit signed value, can't be used for values over 32767)  
c - char (8-bit signed value, can't be used for values over 127)  
x - hex (stored as long)  
"-" (dash character) means that value is unused/not-parsed (this way memory won't be allocated for values from that column)  

# How to cast returned values appropriately
By casting to the corresponding format specifier. Let's suppose that we parse the following:  
```cpp
char * csv_str = "my_strings,my_floats\n"
		 "hello,1.1\n"
		 "world,2.2";
		 
CSV_Parser cp(csv_str, /*format*/ "sf"); // s = string, f = float
```

To cast/retrieve the values we can use:  
```cpp
char  **strings = (char**)cp["my_strings"];
float *floats =   (float*)cp["my_floats"];
```

"x" (hex input values), should be cast as "long*" (or unsigned long*), because that's how they're stored. Casting them to "int*" would result in wrong address being computed when using `ints[index]`.  
  
  
# CSV files without header
To parse CSV files without header we can specify 3rd optional argument to the constructor. Example:  
```cpp
CSV_Parser cp(csv_str, /*format*/ "---L", /*has_header*/ false);
```

And then we can use the following to get the extracted values:  
```cpp
long * longs = (long*)cp[3]; // 3 becuase L is at index 3 of "---L" format string
```

# How to check if the file was parsed correctly
Use CSV_Parser.Print function and check serial monitor. Example:  
```cpp
CSV_Parser cp(csv_str, /*format*/ "---L");
cp.Print();
```

It will display parsed header fields, their types and all the parsed values. Like this:  
> CSV_Parser content:  
>   Header:  
>      my_strings | my_longs | my_ints | my_chars | my_floats | my_hex | my_to_be_ignored  
>   Types:  
>      char* | long | int | char | float | hex (long) | unused  
>   Values:  
>      hello | 70000 | 140 | 10 | 3.33 | FF0000 | -   
>      world | 80000 | 150 | 20 | 7.77 | FF | -  
>      noice | 90000 | 160 | 30 | 9.99 | FFFFFF | -  

  
# Testing
It was tested with Esp8266.  
`ESP.getFreeHeap()` function was used to check for possible memory leak. 

# To do
Examine how memory space efficient it is (when parsing) and how much memory the code/sketch takes.   
Variable delimiter (instead of hardcoded comma).  


