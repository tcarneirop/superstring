using Printf
using Dates
using Distributed

# Include the worker logic module so its types are available
include("superstring_worker_logic.jl")
using .SuperstringWorkerLogic # Use the module

# The master will directly use SuperstringWorkerLogic.SolverState for its overall state.
# now takes SuperstringWorkerLogic.SolverState
function generate_initial_load_get_subproblems_master_wrapper(
    master_solver_state::SuperstringWorkerLogic.SolverState, # Uses the common SolverState type
    current::String,
    used::Vector{Bool},
    level::Int,
    cutoff_level::Int,
    pool_subproblems::Vector{SuperstringWorkerLogic.Subproblem} # Defined in worker logic, used by master
)
    # The actual recursive function is now directly in SuperstringWorkerLogic
    SuperstringWorkerLogic.generate_initial_load_get_subproblems(
        master_solver_state, # Pass the master's SolverState directly
        current,
        used,
        level,
        cutoff_level,
        pool_subproblems
    )
end


function julia_main(args::Vector{String})::Cint
    
    ### parameters checking
    if length(args) != 3
        @error "Usage: julia <script_name>.jl reads.txt cutoff_level num_workers"
        return 1
    end

    filename = args[1]
    cutoff_level = parse(Int, args[2]) 
    num_workers_requested = parse(Int, args[3]) # Parse num_workers from command line

    # Master uses the common SuperstringWorkerLogic.SolverState
    master_solver_state = SuperstringWorkerLogic.SolverState() 

    # Read dna fragment from file
    ## this is our instance
    try
        open(filename, "r") do io
            for line in eachline(io)
                push!(master_solver_state.reads, strip(line))
            end
        end
    catch e
        @error "Error opening or reading file $filename: $e"
        return 1
    end

    master_solver_state.num_reads = length(master_solver_state.reads)

    println("\n############## Problem Read -- OK ##############")
    println("\nNum reads: $(master_solver_state.num_reads)")


    
    # Load generation (on master)
    ### Or what I call ''the initial search''
    #### Responsible for evaluating a fraction of the solution space
    ###### Keeping feasible and valid  and incomplete solutions on a pool.
    ####### In turn, these subproblems can be seen as disjoint fractions of the solution space.

    initial_string = ""
    used = fill(false, master_solver_state.num_reads)
   
    pool_of_subproblems = Vector{SuperstringWorkerLogic.Subproblem}() 
    sizehint!(pool_of_subproblems, SuperstringWorkerLogic.POOL_SIZE) 


    initial_load_start_time = now()
    ##### Lembrando!! -- serial, partial search, generating the pool of subproblems
    for i in 1:master_solver_state.num_reads
        used[i] = true
        initial_string = master_solver_state.reads[i]
        generate_initial_load_get_subproblems_master_wrapper( # Calls the wrapper
            master_solver_state, initial_string, used, 1, cutoff_level, pool_of_subproblems
        )
        used[i] = false 
    end
    initial_load_end_time = now()

    println("\nCutoff depth: $(cutoff_level), Num subproblems: $(master_solver_state.num_subproblems_generated)")
    println("Initial Load Generation Time: $(canonicalize(Dates.Millisecond(initial_load_end_time - initial_load_start_time)))")

    # End of Initial load generation (on master)


    # --- Distributed Setup ---
    workers_to_add = 0
  
    ### And... some debug info
    if num_workers_requested > 0
        workers_to_add = num_workers_requested
        println("MASTER: Master (PID $(myid())): Attempting to add $(workers_to_add) worker processes...")
        addprocs(workers_to_add) 
        println("MASTER: Master (PID $(myid())): After addprocs, current nprocs: $(nprocs()). Workers: $(workers())")
        
        println("MASTER: Master (PID $(myid())): Distributing superstring worker logic module...")
        @everywhere include("superstring_worker_logic.jl") 
        println("MASTER: Master (PID $(myid())): Worker logic module distributed.")
    else
        @error "Number of workers needs to be > 0."
        return 1
    end
    # --- End Distributed Setup ---



    # --- Parallel Solve Phase ---
    solve_start_time = now()
    
    master_current_best_len = master_solver_state.best_len 
    master_current_best_result = master_solver_state.best_result
    master_total_solutions_count = master_solver_state.num_solutions
    master_total_overlap_verifications_count = master_solver_state.num_overlap_verifications

    if nprocs() > 1 && !isempty(pool_of_subproblems)
        println("MASTER: Distributing $(length(pool_of_subproblems)) subproblems to workers...")
        
        ##
        # In the reduction, a is the best result so far and b the new result from the worker.
        # So, after each iteration it verifies if the solution returned, a 4-tuple is better than the current one. 
        ##
        reduction_function = (a, b) -> begin
            new_best_len = min(a[1], b[1])
            new_best_result = (a[1] < b[1]) ? a[2] : b[2] 
            new_solutions = a[3] + b[3]
            new_verifications = a[4] + b[4]
            (new_best_len, new_best_result, new_solutions, new_verifications)
        end

        ####
        #Important -- the way things are implemented is not the most efficienty but it is straightforward.
        #           --- A more efficient way would be - after each iteration the worker gets the best solution
        #             --- the master has, which would make the pruning much more efficient.
        ####
        aggregated_results = @distributed (reduction_function) for sub_problem in pool_of_subproblems
            SuperstringWorkerLogic.solve_subproblem_on_worker(
                sub_problem, 
                master_solver_state.reads, 
                master_solver_state.num_reads,
                cutoff_level, 
                master_current_best_len 
            )
        end

        ### What do we get from the search?
        #### [1] - the length of the optimal solution
        #### [2] - one optimal solution (There might be planty of optimal solutions)
        #### [3] - number of complete solutions found by the distributed search
        #### [4] - the number of string overlap operations performed -- the most expensive one    
        master_current_best_len = aggregated_results[1] 
        master_current_best_result = aggregated_results[2]
        master_total_solutions_count += aggregated_results[3] 
        master_total_overlap_verifications_count += aggregated_results[4] 

    else # if the pool is empty or there are no workers
        @error "Subproblem pool is empty."
        return 1
    end
    solve_end_time = now()

    # Update the master's final solver_state with the aggregated results
    master_solver_state.best_len = master_current_best_len
    master_solver_state.best_result = master_current_best_result
    master_solver_state.num_solutions = master_total_solutions_count
    master_solver_state.num_overlap_verifications = master_total_overlap_verifications_count


    println("\nBest superstring: $(master_solver_state.best_result)")
    println("Length: $(master_solver_state.best_len)")
    println("Number of stringcomp calls: $(master_solver_state.num_overlap_verifications)")
    println("Number of complete solutions found: $(master_solver_state.num_solutions)")
    println("Subproblem Solving Time: $(canonicalize(Dates.Millisecond(solve_end_time - solve_start_time)))")

    try
        if nprocs() > 1
            println("MASTER: Master (PID $(myid())): Cleaning up worker processes...")
            rmprocs(workers())
            println("MASTER: Master (PID $(myid())): Worker cleanup complete. Final nprocs: $(nprocs()).")
        end
    catch e
        println(stderr, "Error during worker cleanup: $e")
    end

    return 0
end

if abspath(PROGRAM_FILE) == @__FILE__
    julia_main(ARGS)
end