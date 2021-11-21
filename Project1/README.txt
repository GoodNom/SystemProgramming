20171630 남주형

1. 프로젝트 목표

이 프로그램을 SIC/XE머신 구현 이전 단계로 어셈블러, 링크, 로더들을 실행하게 될 셸(shell)과 
컴파일을 통해서 만들어진 object코드가 적재 되고 실행될 메모리공간과 
mnemonic (ADD, COMP, FLOAT, etc …)을 opcode값으로 변환하는 OPCODE 테이블과 
관련 명령어들을 구현하는 프로그램이다.

2. 프로그램 실행 방법

해당 폴더의 압축을 풀고 디렉토리에 들어간 후 make를 입력한다.
프로그램 실행은
# ./20171630.out
를 입력하면 된다.

3. 사용 가능한 명령어

h[elp]
d[ir]
q[uit]
hi[story]
du[mp] [start,end]
e[dit] address, value
f[ill] start, end, value
reset
opcode mnemonic
opcodelist

※ 모든 명령어는 소문자로만 입력 가능하며,
   예외로 'opcode mnemonic' 명령어에서 mnemonic 부분은 대문자를 사용한다.