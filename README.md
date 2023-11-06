## GPA65
Convert cc65 debug data to GPA symbol files for logic analyzers.

Usage: gpa65 [options] INPUT.dbg OUTPUT.sym

Program options:
  -w        Ignore source data warnings
  -e        Ignore source data errors
  --help    Display this message and exit

Output options (default all):
  -s        Print Segments  (Sections)
  -f        Print Scopes    (Functions)
  -u        Print Labels    (User)
  -l        Print Segments  (Source lines)

