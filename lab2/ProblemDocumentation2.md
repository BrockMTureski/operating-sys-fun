ELEC 377 Lab 2
Emma Flindall 20212230; Brock Tureski 20151968
18-10-2022

__Problem Statement:__
The purpose of this lab was to create a system with parallel threads that utilize semaphores to achieve synchronization. The system simulates a distributed system monitor.

__Describe how you solved the problem:__

`main`

main begins by unlinking semaphores if theyre linked, initializing thread info and initializing the shared memory segment. monitor threads are then created using a for loop in the range of the variable num_monitor_threads, these threads then goes through and shares thread information to the shared memory segament. The main function then goes on to initialize a single reader thread to read information from the machines and summarize it in the summary structure pointed to by the shared memory segment. The main function then initializes a printer thread to relay machine info to the console

`monitor_update_status_entry`

this function begins by using the wait function on access_stats and mutex, simultaneously checking if the machine_stats and shared_memory memory segments are being used by a different thread and if not being used both those memory segments are locked from other threads. monitor count is then incremented in shared memory and  mutex is then posted. monitor data is then stored to the machine_stats structure from the current thread and is then outputted to console and machine is marked as unread as a flag for the reader flag to read its data. monitor count is checked to ensure it is not 0, if 0 error is thrown and program is exited. mutex semaphore gets the wait treatment so that monitor count can be decremented as the critical section is exited. finally access_stats and mutex semaphores is posted exiting monitor critical section.

`reader`

reader thread function enters while loop that continues until all monitors threads are done. the first thing this loop does is go through an index of all of the machines and checks the shared memory segment to see if the machine at each index has raised a flag for the reader thread to update values, if flag is raised takes in some information about the monitor thread from the shared memory segment. ouptuts total information about the machines. then the access_summary semaphore is locked and summary section of shared memory is updated. the function then checks if there are any monitors remaining, if theres not more_updates is set to 0 and the thread exits.

`printer_thread`

printer thread has same while loop setup as reader function, all the thread does is go through and print out summary stats from the shared memory segment to the console and then exit once no monitors are left.

__Describe testing:__


