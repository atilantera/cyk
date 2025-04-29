In computing, a newline,[1] also known as a line break or end-of-line (EOL)
character, is a special character or sequence of characters signifying the end
of a line of text. The name comes from the fact that the next character after
the newline will appear on a new line—that is, on the next line below the text
immediately preceding the newline. The actual codes representing a newline vary
across operating systems, which can be a problem when exchanging data between
systems with different representations.
There is also some confusion whether newlines terminate or separate lines. If a
newline is considered a separator, there will be no newline after the last line
of a file. The general convention on most systems is to add a newline even
after the last line, i.e. to treat newline as a line terminator. Some programs
have problems processing the last line of a file if it is not newline
terminated. Conversely, programs that expect newline to be used as a separator
will interpret a final newline as starting a new (empty) line.  In text
intended primarily to be read by humans using software which implements the
word wrap feature, a newline character typically only needs to be stored if a
line break is required independent of whether the next word would fit on the
same line, such as between paragraphs and in vertical lists. See hard return
and soft return.  Software applications and operating systems usually represent
a newline with one or two control characters:
* Systems based on ASCII or a compatible character set use either LF (Line
feed, '\n', 0x0A, 10 in decimal) or CR (Carriage return, '\r', 0x0D, 13 in
decimal) individually, or CR followed by LF (CR+LF, 0x0D 0x0A). These
characters are based on printer commands: The line feed indicated that one
line of paper should feed out of the printer, and a carriage return indicated
that the printer carriage should return to the beginning of the current line.
Some rare systems, such as QNX before version 4, used the ASCII RS (record
separator, 0x1E, 30 in decimal) character as the newline character. 
