using Distributed # Essential for parallel computing functions

# --- Main execution block ---
# This block runs for EVERY Julia instance that loads this file directly.
if abspath(PROGRAM_FILE) == @__FILE__
    println("DEBUG START: Process $(myid()) entered main execution block (Main script).")
    println("DEBUG START:   Program file: $(PROGRAM_FILE)")
    println("DEBUG START:   @__FILE__: $(@__FILE__)")
    println("DEBUG START:   Current ARGS: $(ARGS)")
    println("DEBUG START:   Current nprocs(): $(nprocs())")

    # This is a robust check to ensure only the *original* master process proceeds.
    # It accounts for cases where Julia might internally re-run the script in unexpected ways.
    julia_worker_id_env = get(ENV, "JULIA_WORKER_ID", "")
    is_launched_worker = !isempty(julia_worker_id_env) && julia_worker_id_env != "1"
    is_master_process_candidate = (myid() == 1)
    is_true_master = is_master_process_candidate && !is_launched_worker

    println("DEBUG START:   JULIA_WORKER_ID environment variable: '$(julia_worker_id_env)'")
    println("DEBUG START:   is_launched_worker (based on ENV): $(is_launched_worker)")
    println("DEBUG START:   is_master_process_candidate (based on myid()==1): $(is_master_process_candidate)")
    println("DEBUG START:   is_true_master (combined check): $(is_true_master)")


    if is_true_master # This condition should be true ONLY for the original, true master process
        # MASTER PROCESS LOGIC
        println("\nMASTER DEBUG: Master (PID $(myid())): Initiating hello world sequence.")

        # Parse command-line argument for number of workers
        if length(ARGS) != 1
            println(stderr, "Usage: julia helloworld_main.jl <num_workers>")
            exit(1)
        end
        num_workers_to_add = parse(Int, ARGS[1])

        if num_workers_to_add > 0
            println("MASTER DEBUG: Master (PID $(myid())): Attempting to add $(num_workers_to_add) worker processes...")
            
            # --- CRITICAL FIX: NO exeflags for the script file itself ---
            # Just add the workers. They will connect normally.
            addprocs(num_workers_to_add) 
            # --- END CRITICAL FIX ---

            println("MASTER DEBUG: Master (PID $(myid())): After addprocs, current nprocs: $(nprocs()). Workers: $(workers())")
            
            # Now, after workers have successfully connected and are properly identified,
            # include the separate module with worker functions on all of them.
            println("MASTER DEBUG: Master (PID $(myid())): Distributing worker functions module...")
            @everywhere include("helloworld_worker_functions.jl") # This makes the module available on workers.
            println("MASTER DEBUG: Master (PID $(myid())): Worker functions module distributed.")
        else
            println("MASTER DEBUG: Master (PID $(myid())): Running without additional workers.")
        end

        println("\n--- Hello World Messages ---")
        println("Master says: Hello from master! My ID is $(myid()). Total processes: $(nprocs())")

        # Now, you can call functions from the `HelloWorkerFunctions` module on workers.
        @everywhere HelloWorkerFunctions.say_hello_from_worker()
        
        println("--- End Hello World Messages ---\n")


        # Clean up workers when done
        try
            if nprocs() > 1
                println("MASTER DEBUG: Master (PID $(myid())): Cleaning up worker processes...")
                rmprocs(workers())
                println("MASTER DEBUG: Master (PID $(myid())): Worker cleanup complete. Final nprocs: $(nprocs()).")
            end
        catch e
            println(stderr, "Error during worker cleanup: $e")
        end
    else
        # WORKER PROCESS LOGIC
        # This code block is executed by processes that are NOT the true master.
        # These are processes launched by addprocs that simply wait for commands.
        println("WORKER/NON-MASTER DEBUG: Process $(myid())) entered main block's ELSE branch (Worker startup).")
        println("WORKER/NON-MASTER DEBUG: Process $(myid())): JULIA_WORKER_ID: '$(julia_worker_id_env)'")
        println("WORKER/NON-MASTER DEBUG: Process $(myid())): Confirmed as NON-MASTER. Waiting for tasks from Master (PID 1).")
        
        # Workers implicitly wait for tasks from the master after this block.
        println("WORKER/NON-MASTER DEBUG: Process $(myid())): Exiting initial worker block and waiting for master commands.")
    end
    println("DEBUG END: Process $(myid()) finished main execution block.")
end