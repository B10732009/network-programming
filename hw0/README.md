# Network Programming Homework 0 # 

Deadline: Wednesday, 2022/10/06 23:55

#### I. Description  ####

In this homework, you are given a part of a program and a terminal output of two processes executed simultaneously. You should figure out possible context switch points according to the output. 

The left part of page 2 contains a program that periodically reads a number from a file, prints its PID with the number, increases the number by one, and writes the number back to the file. The right part of page 2 shows the output of two processes executed simultaneously (suppose there is only one core). Please analyze the possible situation of every context switch point (marked as (1), (2) ... (9)) and give your answer on page 3. 

The **Executing Block** means the section where the process is paused when a context switch happens. You should answer with “Line x ~ y” which represents “line x is finished and line y must not have been executed”. For example, “Line 6 ~ 7” means that “line 6 is finished while line 7 must not have been executed”, and “Line 10 ~ 6” means that “line 10 is finished while line 6 (next iteration) must not have been executed”. The Description block should contain the reason of your judgement. 

You should give the answer based on the output in page 2. DO NOT execute the program. Besides, there may be more than one answer, and you only need to give one reasonable answer with explanation.

#### II. Submission ####

- You should submit your answer to the E3 system. 
- Only submit the answer page. 
- You should name your file as [student_id].pdf, for example, “0856000.pdf”. ATTENEION! We only accept PDF format. 
- Late submissions are not accepted after the deadline. 
- DO NOT use handwriting. 