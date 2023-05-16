# CSE_305
Projects of Computer architecture lecture in DGIST
  
## Environment
- Ubuntu : 20.04.5 LTS  
- gcc : 9.4.0
  
## RUN

```
$ cd (실행하고 싶은 project)/Code/ 
$ ./run.sh
```
  
각각의 sample assembly code에 따른 결과를 확인 할 수 있다.
  
## Description

### project1
 MIPS(Big-endian) assembly code를 binary code로 변환하는 MIPS ISA assembler를 구현했다. 총 23개의 명령어 지원한다.
 
### project2
MIPS instruction 실행시킬 수 있는 간단한 single-cycle MIPS 에뮬레이터를 구현했다. project1에서 생성한 binary code를 input으로 받아 memory에 로드하고, instruction 실행시키는 에뮬레이터를 구현했다. 또한 instruction이 수행됨에 따라 register와 memory 상태를 출력하도록 구현했다.

### project3
project2의 확장으로, MIPS ISA를 대상으로 다섯 단계(IF, ID, EX, MEM, WB)의 pipeline을 구현했다. Cycle에 따라 register와 memory 그리고 pipeline state register를 출력하도록 구현했다. pipeline에서 발생하는 hazard를 해결하기 위해 pipeline state register를 이용해 다음과 같이 해결했다. 

#### A. Data Hazard
 - Forwading : ‘MEM/WB to EX’, ‘EX/MEM to EX’, ‘MEM/WB to MEM‘
 - 1 stall : load-use 
 
#### B. jump Control Hazard
 - 1 stall 
 
#### C. Branch Control Hazard
ID stage branch forwarding이 아닌 MEM 단계에서 분기 결정되는 것으로 구현
 
 - Always Taken success : 1 stall
 - Always Taken fail : 3 flushing

 - Always Not Taken success : None
 - Always Not Taken fail : 3 flushing

  
  
  
