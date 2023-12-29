# execvp를 활용한 shell구현


간단한 설명
project.pdf
1. 저희 팀이 현재 구현해야 하는 것을 설명해주는 파일이며, 이 project에서는 2명이 팀을 이루어 진행함
2. 우분투에서 실행함

code 부분 설명
1. code에 주석을 추가함 
2. fork함수를 사용해 child process를 생성하고, child process에서 execvp()를 이용함
3. code에선 child process를 실행하는 부분을 보면 직접 구현한 부분을 먼저 실행하고 구현하지 않은 부분은 2에서 말한 execvp를 이용
4. 구현한 부분 -> redirection, pwd, cd, ls와 ls -l을 구현


결론
-> 2명에서 작업을 하다보니 보기 code를 최대한 간결하게 짤려고 함
