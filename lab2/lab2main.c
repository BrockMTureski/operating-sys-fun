//+
// File: lab2main.c
//
// Purpose: The main function and the initial functions
//          to be completed as part of ELEC 277 Lab 2.
//
// Copyright 2022 Iffy Maduabuchi, Thomas Dean
//-
//ELEC 377 Lab 2
//Emma Flindall 20212230; Brock Tureski 20151968
//18-10-2022
#define CLOCKS_PER_SEC = 1000000

#include "common.h"
#include <stdio.h>
#include <time.h>



struct monitor_thread_info monitor_threads[MAX_MACHINES];
struct printer_thread_param printer_param;
struct reader_thread_param reader_param;
char * progname;

//---------------------------------------------------
// Semaphores
//---------------------------------------------------

const char * ACCESS_STATS_NAME = "/access_stats";
const char * ACCESS_SUMMARY_NAME = "/access_summary";
const char * MUTEX_NAME = "/mutex";

sem_t * access_stats;
sem_t * access_summary;
sem_t * mutex;

void init_shared_mem_seg( struct shared_segment * shmemptr );
void * reader_thread(void * parms);
void * printer_thread(void * parms);


// shared memory object
struct shared_segment shared_memory;

int main(int argc, char * argv[]){

    // save executable name in global for error messages;
    progname = argv[0];

    // seed the random generator
    srand((unsigned long)time(NULL));

     //check paramters
    if (argc != 3){
        fprintf(stderr,"usage: %s num_monitors printer_delay_millisecs\n", argv[0]);
        exit(1);
    }

    // First argument is number of machine monitors
    int num_monitor_threads = atoi(argv[1]);
    if (num_monitor_threads == 0){
        fprintf(stderr,"Could not convert <%s> to a number\n", argv[1]);
        exit(1);
    }

    testLog('M',"Monitor threads %d", num_monitor_threads);

    if (num_monitor_threads > MAX_MACHINES){
        fprintf(stderr,"Max number of montiros is %d\n", MAX_MACHINES);
        exit(1);
    }

    // Second argument is number of machine monitors
    int printer_delay = atoi(argv[2]);
    if (printer_delay == 0){
        fprintf(stderr,"Could not convert <%s> to a number\n", argv[2]);
        exit(1);
    }

    // delete the sempahores if they exist
    sem_unlink(ACCESS_STATS_NAME);
    sem_unlink(ACCESS_SUMMARY_NAME);
    sem_unlink(MUTEX_NAME);
    for (int i = 0; i < num_monitor_threads; i++){
        shared_memory.machine_stats[i].read=1;
    }

    init_shared(&shared_memory);

    testLog('M',"Printer delay (msec) %d",printer_delay);

    // default atrributes for initializing threads
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);

    // ---------------------------------------------------
    // START MONITOR THREADS
    // start a thread for each monitor.
    // the monitor_threads array is used to pass
    // parameters
    // ---------------------------------------------------
    for (int i=0; i < num_monitor_threads; i++) {
        monitor_threads[i].machine_number = i;
        monitor_threads[i].shmemptr = &shared_memory;
	// threaded version
        pthread_create(&(monitor_threads[i].thread_id), &thread_attr, monitor_thread_func, (void *)&(monitor_threads[i]));
    }
    
    // TODO: stage 2
    // start reader thread
    pthread_t reader_id;
    reader_param.num_machines=num_monitor_threads;
    reader_param.shmemptr=&shared_memory;
    pthread_create(&reader_id, &thread_attr, reader_thread, (void *)&(reader_param));

    // TODO: stage 3
    // start printer thread
    pthread_t printer_id; 
    printer_param.num_machines=num_monitor_threads;
    printer_param.shmemptr=&shared_memory;
    pthread_create(&printer_id, &thread_attr, printer_thread, (void *)&(printer_param));

    // ---------------------------------------------------
    // FINISH - use pthread_exit instead of return.
    // When the main program finishes all threads are 
    // terminated. Calling pthread_exit exits the main
    // thread while leaving other threads intact. When
    // all of the threads complete, the program will exit.
    // ---------------------------------------------------
    pthread_exit(0);
}

//******************************************************************************
//******************************************************************************
// Functions for you to write to complete the lab
//******************************************************************************
//******************************************************************************

//+
// Function: init_shared
// This function allocates and initializes the three sempaphores as well
// as initiallizing the shared object to the starting state
//-


void init_shared( struct shared_segment * shmemptr ){
    mutex = sem_open(MUTEX_NAME, O_RDWR | O_CREAT, 0660, 1);
    access_stats = sem_open(ACCESS_STATS_NAME, O_RDWR | O_CREAT, 0660, 1);
    access_summary = sem_open(ACCESS_SUMMARY_NAME, O_RDWR | O_CREAT, 0660, 1);

  // if( mutex = sem_open(MUTEX_NAME, O_RDWR | O_CREAT, 0660, 1) == SEM_FAILED) perror("Error opening semaphore\n");
  // if( access_stats = sem_open(ACCESS_STATS_NAME, O_RDWR | O_CREAT, 0660, 1) == SEM_FAILED) perror("Error opening semaphore\n");
  // if( access_summary = sem_open(ACCESS_SUMMARY_NAME, O_RDWR | O_CREAT, 0660, 1) == SEM_FAILED) perror("Error opening semaphore\n");

	shmemptr -> monitorCount = 0;
}

//+
// Function monitor_update_status_entry
// This functon is called by the monitor thread for each entry in the monitor status
// files. There is a delay before each call based on the last field of each line.
// it updates the entry in shared memory with the data sent from the simulated remote machine.
// reader protocol of reader-writer problem.
//
// Parameters:
//    machine_id - the id of the machine that is updated
//    status_id - the number of the update (counts from 0) used only for information
//    cur_read_stat - pointer to data structure from simulated remote machine
//    shmemptr -  pointer to shared object.
//-

void monitor_update_status_entry(int machine_id, int status_id, struct status * cur_read_stat, struct shared_segment * shmemptr ){
    

    colourMsg(machId[machine_id] ,CONSOLE_GREEN,"Machine %d Line %d: %d,%d,%f,%d,%d,%d",machine_id,status_id,
			     (cur_read_stat->machine_state),
			     (cur_read_stat->num_of_processes),
			     (cur_read_stat->load_factor),
			     (cur_read_stat->packets_per_second),
			     (cur_read_stat->discards_per_second),
                 (cur_read_stat->timestamp));

    //------------------------------------
    //  enter critical section for monitor
    //------------------------------------
    
    
    threadLog('M',"Entering critical monitor section.\n");
    int check,updated;
    updated=0;
    check = sem_wait(mutex);
    if(check==-1) {
        perror("Error: monitor already in critical section\n");
        exit(1);
    } else{
        threadLog('M',"monitor thread loop mutex lock aquired");
    }

    shared_memory.machine_stats[machine_id].read++;
    if(shared_memory.machine_stats[machine_id].read==0){
        check = sem_wait(access_stats);
        if(check==-1) {
            perror("Error: monitor already in critical section\n");
            exit(1);
    }   else {
            threadLog('M',"monitor thread loop access_stats lock aquired");
    }
    }
    shared_memory.monitorCount++;

    check=sem_post(mutex);
    if(check==-1) {
        perror("Error: monitor already in critical section\n");
        exit(1);
    } else{
        threadLog('M',"monitor thread loop mutex unlock aquired");
    }
    //------------------------------------
    // monitor critical section
    //------------------------------------

    
    // store the monitor data
    if(shared_memory.machine_stats[machine_id].machine_state!=cur_read_stat->machine_state){
    shared_memory.machine_stats[machine_id].machine_state=cur_read_stat->machine_state;
    updated=1;
    } if(shared_memory.machine_stats[machine_id].num_of_processes!=cur_read_stat->num_of_processes){
    shared_memory.machine_stats[machine_id].num_of_processes=cur_read_stat->num_of_processes;
    updated=1;
    }if(shared_memory.machine_stats[machine_id].load_factor!=cur_read_stat->load_factor){
    shared_memory.machine_stats[machine_id].load_factor=cur_read_stat->load_factor;
    updated=1;
    }if(shared_memory.machine_stats[machine_id].packets_per_second!=cur_read_stat->packets_per_second){
    shared_memory.machine_stats[machine_id].packets_per_second=cur_read_stat->packets_per_second;
    updated=1;
    }if(shared_memory.machine_stats[machine_id].discards_per_second!=cur_read_stat->discards_per_second){
    shared_memory.machine_stats[machine_id].discards_per_second=cur_read_stat->discards_per_second;
    updated=1;
    }if(shared_memory.machine_stats[machine_id].timestamp!=cur_read_stat->timestamp){
    shared_memory.machine_stats[machine_id].timestamp=cur_read_stat->timestamp;
    updated=1;
    }
    
    // report if overwritten or normal case (Stage 2)
    if(updated==1){
    colourMsg(machId[machine_id],CONSOLE_CYAN,"Machine %d updated.",machine_id);
    colourMsg(machId[machine_id] ,CONSOLE_GREEN,"Machine %d Line %d: %d,%d,%f,%d,%d,%ld",machine_id,status_id,
			     (cur_read_stat->machine_state),
			     (cur_read_stat->num_of_processes),
			     (cur_read_stat->load_factor),
			     (cur_read_stat->packets_per_second),
			     (cur_read_stat->discards_per_second),
                 (cur_read_stat->timestamp));
                 updated=0;
    }
    // mark as unread
    shared_memory.machine_stats[machine_id].read=0;

    //check if monitor count is 0 if 0 exit
    if(shared_memory.monitorCount==0) {
        check=sem_post(access_stats);
        if(check==-1) {
        perror("Error: access_stats sem already unlocked.\n");
        exit(1);
    } else{
        threadLog('M',"error, monitor count is 0");
        exit(1);
    }
    }
    check = sem_wait(mutex);
    if(check == -1) {
        perror("Error: error posting semaphore.\n");
        exit(1);
    } else{
        threadLog('M',"monitor thread loop mutex unlock aquired");
        shared_memory.monitorCount--;
    }
    //------------------------------------
    // exit critical setion for monitor
    //------------------------------------
    check = sem_post(access_stats);
    if(check == -1) {
        perror("Error: error posting semaphore.\n");
        exit(1);
    } else{
        threadLog('M',"monitor thread loop access_stats unlock aquired");
    }

    check = sem_post(mutex);
    if(check == -1) {
        perror("Error: error posting semaphore.\n");
        exit(1);
    } else{
        threadLog('M',"monitor thread loop mutex unlock aquired");
    }
}


// stage 2
/*
 Function: reader_thread
 This thread
        read and calculate the summary for new entries
        takes the pointer to the shared memory segment
        the pointer to the total entries read up to now
        and a pointer to max entries to read
        function should recalculate and update summary information for every new entry
        raise alarms for systems that are down
        update the total entries read
        if total_entries_read no longer less than max entries, should set no more information flag in summary information
*/

void * reader_thread(void * parms){
    struct shared_segment * shmemptr = ((struct reader_thread_param*)parms)->shmemptr;
    int num_machines = ((struct reader_thread_param*)parms)->num_machines;
    int more_updates = 1;
    unsigned read_update_times[MAX_MACHINES];
    int read_machines_state[MAX_MACHINES];
    
    int total_procs, total_pps, total_dps;
    float total_lf;
    
    long summary_checksum;
    time_t start_t,current;
    int start_init=0;
    int updates=0;
    total_procs=0;
    total_lf=0;
    total_pps=0;
    total_dps=0;
    int updatesRead=0;

    start_t=time(NULL);

    
    threadLog('R',"Reader Thread: %d machines", num_machines);
    msleep(1000);
    

    while(more_updates){
        threadLog('R',"Reader Thread loop start", num_machines);

        int check=sem_wait(access_stats);
        if(check==-1){
            perror("ERROR: stats already being accessed");
            exit(1);
        }


        // check for updates to each machine
        for(int i = 0; i < num_machines; i++){
           if(shared_memory.machine_stats[i].read == 0){
                // collect stats for all machines
                read_machines_state[i] = shmemptr->machine_stats[i].machine_state;
                read_update_times[i] = shmemptr->machine_stats[i].timestamp;
                total_procs += shmemptr->machine_stats[i].num_of_processes;
                total_lf += shmemptr->machine_stats[i].load_factor;
                total_pps += shmemptr->machine_stats[i].packets_per_second;
                total_dps += shmemptr->machine_stats[i].discards_per_second;
               
               colourMsg(machId[i] ,CONSOLE_CYAN,"read flag disabled");
               updatesRead++;
               shared_memory.machine_stats[i].read = 1;
           }

        }

        
        //output stats for all machines
        colourMsg('T', CONSOLE_PURPLE, "Total processes = %d", total_procs);
        colourMsg('T', CONSOLE_PURPLE, "Total load factor = %f", total_lf);
        colourMsg('T', CONSOLE_PURPLE, "Total packets per second = %d", total_pps);
        colourMsg('T', CONSOLE_PURPLE, "Total discarded packets per second = %d", total_dps);           
        
        // release stats semaphore
        check=sem_post(access_stats);
        if(check==-1){
            perror("ERROR\n");
            exit(1);
        }

        threadLog('R',"Reader Thread loop  accessing_stats lock released", num_machines);


        //checksum - consume time outside of critical section.
        shmemptr->checksum_seed = gen_checksum_seed();
        summary_checksum = gen_summary_checksum();

        
        //=======================
        // do summary information (state 3)
        //=======================
        
        // lock summary semaphore
        check=sem_wait(access_summary);  
        if(check==-1){
             perror("ERROR\n");
            exit(1);
        }
        
        // write summary checksum
        shmemptr->summary.checksum=gen_summary_checksum();

        // update machine uptime sand last heard
        
        current = time(NULL);

        for(int i = 0; i<num_machines;i++){
            shmemptr->summary.machines_last_updated[i]=read_update_times;
            if(start_init!=1){
                shmemptr->summary.machines_online_since[i]=start_t;
                start_init=1;
            }}

        // calculate new averages
        if(num_machines!=0){
            shmemptr->summary.avg_procs=total_procs/updatesRead;
            shmemptr->summary.avg_lf=total_lf/updatesRead;
            shmemptr->summary.avg_pps=total_pps/updatesRead;
            shmemptr->summary.avg_dps=total_dps/updatesRead;
        }
        // releast summary semaphore
        check=sem_post(access_summary);
        if(check==-1){
            perror("ERROR\n");
            exit(1);
        }
        //=======================
        // are the monitors still running? (Stage 2)
        //=======================
        if(shmemptr->numMonitors == 0){
                more_updates = 0;
        }
       
        
        threadLog('R',"Reader Thread loop end", num_machines);
    }

    pthread_exit(0);
    // not reached.
    return NULL;
}

// stage 3

void * printer_thread(void * parms){
    struct shared_segment * shmemptr = ((struct printer_thread_param*)parms)->shmemptr;
    int print_period = ((struct printer_thread_param*)parms)->print_period;
    int num_machines = ((struct printer_thread_param*)parms)-> num_machines;
    int more_updates = 1;
    unsigned cur_uptime;
    unsigned cur_time;


    
    threadLog('P',"Printer Thread: delay %d", print_period);
    
    while(more_updates){
        
        // sleep for print_period
        msleep(print_period);
        
        // aquire summary mutex
        int check=sem_wait(access_summary);
        if(check==-1){
            perror("ERROR: summary semaphore already raised\n");
            exit(1);
        }
        
        // get current time
        clock_t end_t = time(NULL);
        // printe summary
        threadLog('P',"Printer Step");

        printf("[%u] SUMMARY INFORMATION\n", get_current_unix_time());
        printf("MACHINE | UP | UPTIME                 | LAST UPDATE  \n");
        printf("-----------------------------------------------------\n");
        
        for (int i = 0; i < num_machines; i++){
            long uptime=(long)(end_t - shmemptr->summary.machines_online_since[i]);
            long lastupdate=(long)(shmemptr->summary.machines_last_updated[i]);

        printf("%d        %d   %ld                      %ld\n",i+1,shmemptr->summary.machines_state[i],uptime,lastupdate);
        }
        
        // release summary mutex
        check=sem_post(access_summary);
        if(check==-1){
            perror("ERROR\n");
            exit(1);
        }

        //Are the monitors still running.
        if(shmemptr->numMonitors == 0){ 
            more_updates=0;
        }
    }
    
    pthread_exit(0);
    // not reached.
    return NULL;
}
