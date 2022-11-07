//+
// File: lab2main.c
//
// Purpose: The main function and the initial functions
//          to be completed as part of ELEC 277 Lab 2.
//
// Copyright 2022 Iffy Maduabuchi, Thomas Dean
//-

#include "common.h"


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
    // START MONITOR TREADS
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
    
    // ---------------------------------------------------
    // start reader thread
    // ---------------------------------------------------

    reader_param.shmemptr = &shared_memory;
    reader_param.num_machines = num_monitor_threads;
    
    pthread_t reader_id;
    pthread_create(&reader_id,&thread_attr, reader_thread, (void*)&reader_param);
    
    // ---------------------------------------------------
    // start printer thread
    // ---------------------------------------------------

    printer_param.shmemptr = & shared_memory;
    printer_param.print_period = printer_delay;
    printer_param.num_machines = num_monitor_threads;
    
    pthread_t printer_id;
    pthread_create(&printer_id,&thread_attr, printer_thread, (void*)&printer_param);

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

        // initialize reader-writer mutex 
        if ((access_stats = sem_open(ACCESS_STATS_NAME, O_RDWR|O_CREAT, 0660, 1)) == SEM_FAILED ) {
                perror("Error initializing reader/writer mutex!");
                exit(1);
        }

        if ((mutex = sem_open(MUTEX_NAME, O_RDWR | O_CREAT, 0660, 1)) == SEM_FAILED){
                perror("Error initializing access_stats mutex!");
                exit(1);
       }

        if ((access_summary = sem_open(ACCESS_SUMMARY_NAME, O_RDWR | O_CREAT, 0660, 1)) == SEM_FAILED){
                perror("Error initializing access_stats mutex!");
                exit(1);
        }

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

    colourMsg(machId[machine_id] ,CONSOLE_GREEN,"Machine %d Line %d: %d,%d,%f,%d,%d",machine_id,status_id,
			     (cur_read_stat->machine_state),
			     (cur_read_stat->num_of_processes),
			     (cur_read_stat->load_factor),
			     (cur_read_stat->packets_per_second),
			     (cur_read_stat->discards_per_second));

    //------------------------------------
    //  enter critical section for monitor
    //------------------------------------

    if (sem_wait(mutex) == -1){
        perror("monitor entry critical section failed");
	    exit(1);
    }

    shmemptr -> monitorCount++;

    // if we are the first one in, check to make
    // sure the summarizer is not here.
    if (shmemptr -> monitorCount == 1){
        if (sem_wait(access_stats) == -1) {
            perror("first monitor failed to get mutex");
            exit(1);
        }
    }


    if (sem_post(mutex) == -1){
        perror("monitor exit critical section failed");
	exit(1);
    }
    colourMsg(machId[machine_id] ,CONSOLE_GREEN,"Enter Critical Section");

    //------------------------------------
    // monitor critical section
    //------------------------------------

    
    // store the monitor data
    shmemptr -> machine_stats[machine_id].machine_state = cur_read_stat->machine_state;
    shmemptr -> machine_stats[machine_id].num_of_processes = cur_read_stat->num_of_processes;
    shmemptr -> machine_stats[machine_id].load_factor = cur_read_stat->load_factor;
    shmemptr -> machine_stats[machine_id].packets_per_second = cur_read_stat->packets_per_second;
    shmemptr -> machine_stats[machine_id].discards_per_second = cur_read_stat->discards_per_second;
    shmemptr -> machine_stats[machine_id].timestamp = cur_read_stat->timestamp;

    
    // report if overwritten or normal case
    if (!shmemptr->machine_stats[machine_id].read){
        colourMsg(machId[machine_id], CONSOLE_RED,"Overwote status before it could be read");
    } else {
        colourMsg(machId[machine_id], CONSOLE_GREEN,"Update Done");
    }
    
    // mark as unread
    shmemptr->machine_stats[machine_id].read = 0;

    //------------------------------------
    // exit critical setion for monitor
    //------------------------------------

    if (sem_wait(mutex) == -1){
        perror("monitor entry critical section");
	exit(1);
    }

    shmemptr -> monitorCount--;

    // if we are the last one out, signal
    // the summarizer.
    if (shmemptr -> monitorCount == 0){
	if (sem_post(access_stats) == -1) {
	   perror("first monitor failed to get mutex");
	   exit(1);
	}
    }

    if (sem_post(mutex) == -1){
        perror("monitor entry critical section");
	exit(1);
    }
    colourMsg(machId[machine_id] ,CONSOLE_GREEN,"Exit Critical Section");

}


/*+
 Function: reader_thread
 This thread
        read and calculate the summary for new entries
        takes the pointer to the shared memory segment, the number of machines.w
        The function should recalculate and update summary information for every new entry
        raise alarms for systems that are down
-*/

void * reader_thread(void * parms){
    struct shared_segment * shmemptr = ((struct reader_thread_param*)parms)->shmemptr;
    int num_machines = ((struct reader_thread_param*)parms)->num_machines;
    int more_updates = 1;
    unsigned read_update_times[MAX_MACHINES];
    int read_machines_state[MAX_MACHINES];
    
    int total_procs, total_pps, total_dps;
    float total_lf;
    
    long summary_checksum;
    
    threadLog('R',"Reader Thread: %d machines", num_machines);
    msleep(1000);
    
    while(more_updates){
        
        // aquire stats semaphore
        threadLog('R',"Reader Thread loop start", num_machines);

        if (sem_wait(access_stats) == -1){
            perror("reader locking accessing_stats");
            exit(1);
        }
        threadLog('R',"Reader Thread loop accessing_stats lock aquired", num_machines);

        // check for updates to a given machine
        // collect stats for all machines
        total_procs = 0;
        total_pps = 0;
        total_dps = 0;
        total_lf = 0.0;
        
        for (int i = 0; i < num_machines; i++){
            
            
            // if new data
            if (!shmemptr -> machine_stats[i].read){
                // mark as read
                shmemptr -> machine_stats[i].read = 1;
                            
                // copy status to summary
                read_machines_state[i] = shmemptr->machine_stats[i].machine_state;

                // warn if machine is offline
                if (shmemptr -> machine_stats[i].machine_state==0){
                    colourMsg('R',CONSOLE_RED, "ALERT: Machine %d is down", i);
                }
                
                // copy update time to summary.
                read_update_times[i] = shmemptr->machine_stats[i].timestamp;
                
            }
            
            // accumulate values for summary
            total_procs += shmemptr->machine_stats[i].num_of_processes;
            total_lf += shmemptr->machine_stats[i].load_factor;
            total_pps += shmemptr->machine_stats[i].packets_per_second;
            total_dps += shmemptr->machine_stats[i].discards_per_second;
        }

        // release stats semaphore
        if (sem_post(access_stats) == -1){
            perror("reader releasing accessing_stats");
            exit(1);
        }
        threadLog('R',"Reader Thread loop  accessing_stats lock released", num_machines);


        //checksum - consume time outside of critical section.
        shmemptr->checksum_seed = gen_checksum_seed();
        summary_checksum = gen_summary_checksum();

        
        //=======================
        // do summary information
        //=======================
        
        // lock summary semaphore
        if (sem_wait(access_summary) == -1){
            perror("reader releasing accessing_summary");
            exit(1);
        }
        threadLog('R',"Reader Thread loop  access_summary lock aquired", num_machines);
        
        // write summary checksum
        shmemptr->summary.checksum = summary_checksum;
        
        // update machine uptime sand last heard
        for(int i=0; i < num_machines; i++){
            if (!shmemptr->summary.machines_state[i] && read_machines_state[i]){
                shmemptr->summary.machines_online_since[i] = read_update_times[i];
            }
	    if (!read_machines_state[i]){
		shmemptr->summary.machines_online_since[i] = 0;
	    }
            shmemptr->summary.machines_state[i] = read_machines_state[i];
	    shmemptr->summary.machines_last_updated[i] = read_update_times[i];
        }
        
        // new averages
        shmemptr->summary.avg_procs = total_procs / num_machines;
        shmemptr->summary.avg_lf = total_lf / (float) num_machines;
        shmemptr->summary.avg_pps = total_pps / num_machines;
        shmemptr->summary.avg_dps = total_dps / num_machines;
        
        // releast summary semaphore
        if (sem_post(access_summary) == -1){
            perror("reader releasing accessing_summary");
            exit(1);
        }
        threadLog('R',"Reader Thread loop  access_summary lock released", num_machines);
        
        // are the monitors still running?
        more_updates = shmemptr -> numMonitors > 0;
        threadLog('R',"Reader Thread loop end", num_machines);
    }
    
    pthread_exit(0);
    // not reached.
    return NULL;
}

/*+
 Function: printer_thread
 This thread
        Periodially prints the status of the machine=.
        takes the pointer to the shared memory segment, the print period and the number of machines.
-*/

void * printer_thread(void * parms){
    struct shared_segment * shmemptr = ((struct printer_thread_param*)parms)->shmemptr;
    int print_period = ((struct printer_thread_param*)parms)->print_period;
    int num_machines = ((struct printer_thread_param*)parms)-> num_machines;
    int more_updates = 1;
    

    
    threadLog('P',"Printer Thread: delay %d", print_period);
    
    while(more_updates){
        msleep(print_period);
        // aquire summary mutex
        if (sem_wait(access_summary) == -1){
            perror("printer locking accessing_summary");
            exit(1);
        }
        threadLog('P',"access_summary locked");
        
        unsigned cur_uptime;
        unsigned cur_time = get_current_unix_time();
	printf("cur time = %u\n", cur_time);
        
        // printe summary
        threadLog('P',"Printer Step");

        printf("[%u] SUMMARY INFORMATION\n", get_current_unix_time());
        printf("MACHINE | UP | UPTIME                 | LAST UPDATE  \n");
        printf("-----------------------------------------------------\n");
        
        for (int i = 0; i < num_machines; i++){
            printf("%7d", i);
            printf("| %2s |", (shmemptr->summary.machines_state[i]) ? "Y" : "N");
            if (shmemptr->summary.machines_state[i]){
                cur_uptime = cur_time - shmemptr->summary.machines_online_since[i];
                printf("                   %4u |", cur_uptime );
            } else {
                printf("  -----------------     |");
            }

            if ( shmemptr->summary.machines_last_updated[i] == 0 ) printf(" ------------" );
            else printf(" %12u ", shmemptr->summary.machines_last_updated[i] );
            
            printf("\n");
            
        }
        
        // release summary mutex
        if (sem_post(access_summary) == -1){
            perror("printer locking accessing_summary");
            exit(1);
        }
        threadLog('P',"access_summary released");

        //check if moare updates
        more_updates = shmemptr -> numMonitors > 0;
    }
    pthread_exit(0);
    // not reached.
    return NULL;
}
