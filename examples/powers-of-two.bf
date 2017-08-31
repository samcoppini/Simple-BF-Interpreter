[
Outputs arbitrarily many powers of two.
Never stops on its own, must be killed by the user.
Written by Sam Coppini
]

Sets up the tape and enters the main loop
++++++++++>>+>+[

Go through the tape and output each digit as its ascii equivalent
[+++++[<++++++++>-]<.>++++++[<-------->-]+<<]

Prints a newline and moves to the last digit
<.>>>

Starts the loop of doubling each digit in the number carrying if need be
[

Move a doubled copy of the digit into a temp cell
-<[>++<-[>++<-[>++<-[>++<-

If the doubled number will be greater than nine we carry the one to the
next cell and subtract the doubled number by ten
[>--------<-[>++<-]>>>[-]++<<<]]]]]

After doubling move the temp cell to the digit cell
>[<+>-]

Move to the next digit
+>>]

After finishing the multiply we move to the first digit and loop back to near
the beginning of our program to print the number
<<]
