[
Outputs arbitrarily many powers of two.
Never stops on its own, must be killed by the user.
Written by Sam Coppini
]

Sets up the tape and enters the main loop
>>+>+[

Go through the tape and output each digit as its ascii equivalent
[<<++++++[>++++++++<-]>.<++++++[>--------<-]<]

Print a newline and moves to the first digit
++++++++++.[-]>>>

Moves a doubled copy of the digit into a temp cell
[<[<++>-]

Checks to see if the doubled digit is greater than ten
<[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-

If the digit is greater than ten carry the one to the next cell
[>[-]<[>+<-]>->>+>>[-]+<<<<<

Move the next digit
]]]]]]]]]]>>>>>]<<<]