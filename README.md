# llvm-qyt
#Abstract:
As the speed of CPU get more and more fast and the number of core in cpu get more and more , server or pc can process more  things at the same time. Whereas the capacity of main memory has restriction so that not all program can be processed at the same times in main memory sometimes. And in linux system, we use swap partition  serves as 'backup' RAM. That is when your computer run out of RAM, it will use the swap area as a temporary source of more memory. So sometimes program will generate swapping and bad  swapping performance will affect the whole program performance seriously. So optimize the swap performance is a significant problem in linux system.In practical use, cloud supplier will supplies overburdened service to client because they think all client will not use the services at the same time.In case excess client should be processed at the same time server will incur swapping which can greatly reduces the user experience.Profile-guided optimization(PGO),  also known as profile-directed feedback (PDF) and feedback-directed optimization (FDO)  is a compiler optimization technique in computer programming that uses profiling to improve program runtime performance.There is support for building Firefox using PGO.The HotSpot Java virtual machine (JVM) uses profile-guided optimization to dynamically generate native code. So we can use this idea to fit swap system and give the swap system hint to make the swap sequence correctly and efficiently

#Overview：
Swap in linux is a complex problem.What is swap? The function of Swap space can be simply described as: when the physical memory of the system is not enough, a part of the physical memory needs to be released to be used by the current running program.The released space will be saved temporarily to swap space until the program require it and recover it from Swap saved data into memory. In this way, the system is always swapping when there is not enough physical memory. But as we all know the gap of read-write speed between SSD and DRAM ,and if the disk is HDD there are tremendous read-write speed gap between HDD and DRAM.So sometimes , swap will greatly decrease the performance ,on account of the gaps between DRAM, SSD and HDD speed and IO. Sometimes Swap will happen when the program occupy a deal of memory or there are so many program running simultaneously. So swap will impact the performance seriously and we need to find a way to optimize the performance when the program generate swapping.To amend the core swap mechanism is so hard to fit each condition, I come up with a Profile-guided optimization way inspired by CAMP profiler made by Pohang University of Science and Technology(POSTECH). CAMP is a set of profilers can analysis a program based on llvm compiler. Camp puts forward the concept of context tree and I am inspired by it and produce a DAPHIC-Date Access Pattern HInt Instrumenting  Compiler based on llvm to optimize the performance when workload in heavy swapping environment. In the best of times, performance  will be 3X greater than original swap situation.

#Background:
This chapter will introduce the background of this thesis, briefly. We use PGO to optimize the swap performance  and I will explain the effect to performance when swap happens and the tool named Camp from POSTECH which is the main engine for my profiler. I also need to explain the Camp’s structure and thought which get me a lot of help to make llvm profiler and compiler.Also the  context tree is very important in Camp also used by my profiler.
The effect of swap:
We can know when in linux system the main memory is lack of space it will incur swapping. The system will swap out the data to swap disk temporarily. These swapped data will free the space in main memory and the freed memory space will be used by other data which should be processed now. But pages should be swap out to swap disk will incur I/O which can slow down the performance seriously. When the data in the swap disk will be used now they will be swapped into main memory which will incur I/O again.Also swap system will swap the data in page uint. 
The help from Camp:Camp is a profiler based on llvm which can analyze a program int detail. Llvm is very sensitive to call site and loop,so we can use llvm to analyze the program code which can build a context tree. When the llvm compiler meet a call site or loop, it will think this is a whole context until the end of call site/loop in code. And if the context call other call site/loop the target will be identified into another context which will be the child context to the caller context in the context tree. If a context call many other contexts ,the contexts will be childern context ,so context tree is not a binary tree. Each context have a context id and a local context id. And the context id can be calculated by this equation: parent context id = context id - local id. The root context id is 0 and its local id is 0,too. Local id will be given to a context which means child contexts order. So the child context id is parent context add own local context id.So we can define all contexts in program by different context id. And the context called by different caller will be defined to different context which means if we assume there are three context A,B,C.A call C and B call C, so at this time, C in different caller will have different context id.But different context id may be the same segment of code.So the sample code we get from the bottom code and get the context tree with each context id.

C()
{
	…
}

A()
{
	C();
}

B()
{
	C();
}
main()
{
	A();
	B();
	for()
	{
		…
	}
}
![firest](https://github.com/qiangyuntao2010/llvm-qyt/blob/master/%E5%9B%BE%E7%89%871.png);

#Design and Implementation:

In this chapter I will introduce three parts of DAPHIC. The first part is the design of profiler based on Camp. The second part is the core part, how to analyse the result of the profiler and make two centre algorithms to do context merge and the last one is how to put the result to the  application to tell system how to do a efficient swap in/out.

Profiler redesign based on Camp:
We get a helpful great profiler from POSTECH which is based on llvm.As we all know llvm is produced by c++.Not like the other compiler , llvm-Low Level Virtual Machine is started in 2000 at the University of Illinois at Urbana–Champaign, under the direction of Vikram Adve and Chris Lattner. LLVM can provide the middle layers of a complete compiler system, taking intermediate representation (IR) code from a compiler and emitting an optimized IR. This new IR can then be converted and linked into machine-dependent assembly language code for a target platform. So according these features, the function-PASS in llvm have been raised.The LLVM Pass Framework is an important part of the LLVM system, because LLVM passes are where most of the interesting parts of the compiler exist. Passes perform the transformations and optimizations that make up the compiler, they build the analysis results that are used by these transformations, above all, a structuring technique for compiler code.So Camp is set of passes in llvm to analyze program. Inspired by Camp profiler, I make a set of profiers to analyze the applications. Previously introduced the context id , because llvm is sensitive to call site and loop ,so I hook functions into run time.These functions will be invoked when the call site and loop begin and end so in this functions we can calculate the current context id. According to the global variable current context id we can know which context in the application is processing now.  In the same way to hook call site and loop, we also hook load instructions, store instructions in assembly code and malloc ,realloc ,calloc functions in c language code. So according to the way of hooking functions, we can know the application is in which context and do some calculations in real time. When program call memory allocation functions, the allocated region we name it as a object which contain its virtual address and size. Also we can identify the object by object id which is calculated by context id. So according to the profiler we can analyse the program and the results of execution time of each context and number of object access with each context. So we can use these profiling result to optimize the performance of the program in swap system.

The architecture of DAPHIC is figure 4.1 above.My core of contribution is that combine the concept of PGO and swap system.We profiled the program and get the result, put the profiled result into program through the compiler, in run time we can use the result to system to do the efficient swapping.In the picture we can put the profiled result into hint generator and produce the hint by profiled result. Insert the hint when context begins and tell the system hint by system call. So when context changes, the system will get a hint by system call to tell the system which objects the context will access immediately so that the swap system according to the hint will do efficient swap process. But the problem is there are so many call sites and loops, the number of context change times is so high .If we invoke system call when contexts change, the invoking of system call’s overhead will be so high. But if we decrease the number of system call invoking times the accuracy of hint will go down so to trade off the overhead and accuracy is the first problem I need to solve. First, to decrease the overhead of invoking system call we decide to do context merge. Because in my system we need to invoke system call when context change. So if we merge the contexts which have the same data access pattern and if the context's execution time is low, we can merge the context to its parent context and the system will invoke system call once. So this way will decrease the context change overhead definitely. We test effect of the way of context merge with SPEC2006 milc workload .
(result of context change)
So according to this graph, we can know merging the number of context will decrease the overhead of invoking the system call.
But by merging the context , though decrease the number of invoking system call , the accuracy of hint will go down. So the second challenge is how to send an correct hint. We can think a program can be divided into many phases and each phase has its data access pattern , so we need to merge the contexts which have the similar data access pattern. To determine what is the similar data access pattern, we come up with two algorithm to define object similarity and hotness similarity. We can use object similarity and hotness similarity to compare two contexts’ data access pattern similarity.

Object similarity algorithm :

Object similarity(A,B) = conjunction(A,B).len / union(A,B).len* 100

Assume we have two different named A,B. The length of A and B’s object conjunction divide the length of A and B’s object union multiply 100.On the limit situation If two context A,B access the same set of object the object similarity will be 100 ,in turn if two context access completely different set of objects the object similarity will be 0.

Hotness similarity algorithm :
Hotness similarity(A,B) = conjunction = conjunction_of(A,B)
{
    ratio_sum = 0
    for k in conjunction.keys()
{
      		average = A[k] + B[k] / 2
      		diff=(absolute(average - a[k]) + absolute(averge - b[k])) / 2
      		ratio_sum += diff / average
} 
 return 1 / ratio_sum / len(conjunction)
}

The A,B’s conjunction means the conjunction of A and B’s objects. And the key of conjunction means the object id and ‘A[k]’ and ‘B[k]’ mean the count of context A/B access the object, the result can be profiled because I hooking load/store instructions. So we can use this function to define the hotness similarity.The hotness similarity gets bigger ,the contexts have the more similar data access pattern. For a better understanding of   these two algorithm we have an example.

We assume there are three context in the context tree and named context 1, context 2, context 3.Context 1 accessed object a and b and the number of access times are 10 and 5 ,respectively.According to the algorithm we can calculate the object similarity and hotness similarity between context 1 and context 2.And also we can calculate the object similarity and hotness similarity between context 1 and context 3.

The similarity value between context 1 and context 2:
Object_simarlity between context 1 and context 2: 2 / 2 *100 = 100
Hotness_simarlity between context 1 and context 2: 1 / {(1/ 11) + (0.5 / 4.5) / 2} = 9.9

And also calculate the similarity value between context 1 and context 3:
Object_simarlity between context 1 and context 3 : 2 / 3 * 100 = 67
Hotness_simarlity between context 1 and context 3: 1 / {(3.5/ 6.5) + (12.5 / 17) / 2}= 1.57

According to the results, we can judge context 2 is more similar to context1 than context 3.
If we can calculate the object similarity and hotness similarity ,we can do context merge to decrease overhead of invoking system call and guarantee the accuracy of hint. We will merge the contexts which have similar data access pattern. And the merge mechanism is as follow:

If obj_sim(n, n.parent) > O_OFFSET and hotness_sim(n, n.parent) > H_OFFSET
{
Merge(n, n.parent)
}
We can decide O_OFFSET and H_OFFSET manually and test many times to decide the best offset value to do the best trade off. According to many tests, I think O_OFFSET is 1 and H_OFFSET is more than 3 is the best way to have the context merge.

#Syscall Call:
If we get the efficient hint, we should send the hints to system by system call when context begins.And we decide to use madvise system call to send hint as most linux kernel use now. We can send the object’s start virtual address and size to system.In the beginning, I think maybe a new system call will be needed, the evaluation result is  good enough, so I decide to use madvise finally.  

Evaluation:
This chapter will evaluate proposed scheme in this chapter.We will use workloads in SPEC 2006 and decrease the physical memory to make swapping. And I will compare the result between original swap performance and  DAPHIC optimized swap performance.The test environment is Intel Xeon E7-8837 2.76GHz cpu with single thread, DDR3 main memory, 125 GB sata ssd as swap partition in linux kernel 4.14.0 version. While to test swapping ,we decrease the physical memory to make swap.

First we test the 433.milc workload in SPEC2006 and we merge the context by execution time is small than 0.34s Object similarity is 1 and  hotness similarity is more than 3.We test milc compiled by DAPHIC and original, and the result is as follow.

According to the figure we can see in the full memory situation daphic version have 20% overhead because we need to search object’s virtual memory address and size in real time on account of the virtual memory address could change every run time. And the milc workload need 600M-650M memory , the number of object is 76 and the context change time is about 0.2 billion.But we can see if the memory size is 500M the runtime will be nearly same.When available memory is 75% of working set, DAPHIC is 2.806x faster and 2.975x faster in 66% available memory size.

And to test the universality of DAPHIC we also test 462.libquantum workload in SPEC 2006.Libquantum uses about 60M size memory.And the number of object is 10 the number of context time is about 10 thousand so DAPHIC show almost no overhead in full-memory.When available memory is 90% of working set, DAPHIC is 1.211x faster, 1.1x faster in 83% available memory size,2.526x faster in 78% available memory size.

#Conclusion
Swap in linux system is a entangled issue but my contribution is combine swap and PGO correctly.And DAPHIC is really fit to the situation that we need to run some programs many times. And at the same time DAPHIC is compiler based on PGO so if the object is more disperse the performance can get better. And we also test two characteristic of workload, the milc workload has more objects and get 3x performance than original one when decrease physical memory to 400M, in turn the libquantum workload has few object than so the optimized performance is ?.DAPHIC is a compiler based on the thought of combination of swap and PGO and produced based on llvm.

Discussion:
In my opinion, DAPHIC have a shortcoming is that when the program just have super little object and each object occupies a large region in memory, but at some point the program do not need to use the whole region of one object, so propose the whole object is maybe can cause redundant swap in/out.Although that kind of workload may not be much but it need to be designed to prevent that situation.And another optimized idea is to use mlock system call to fix a region in main memory to prevent it swapping out if the region is accessed frequently.But the problem is mlock is dangerous , if we use mlock but program may be killed for lacks fo the physical memory. But having a correct use of mlock could optimize the DAPHIC performance a lot.









