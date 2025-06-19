module SuperstringWorkerLogic
using Printf
using Dates 
using Distributed 

# Constants
const MAX_READS = 20
const MAX_LEN = 100
const POOL_SIZE = 10000
const MAX_SUPERSTRING_LEN = MAX_READS * MAX_LEN 

# This single struct will used by both master and workers
mutable struct SolverState
    reads::Vector{String}
    num_reads::Int
    
    # These fields will be local to the worker's search on a subproblem
    num_subproblems_generated::UInt32 
    num_solutions::UInt64
    num_overlap_verifications::UInt64
    best_len::UInt64 
    best_result::String

    # Default constructor (primarily for master's initial state)
    function SolverState()
        new([], 0, 0, 0, 0, typemax(UInt64), "")
    end

    # Constructor for worker's local state (or for recursive generation calls)
    function SolverState(reads::Vector{String}, num_reads::Int, initial_best_len::UInt64)
        new(reads, num_reads, 0, 0, 0, initial_best_len, "")
    end
end

mutable struct Subproblem
    current::String
    used::Vector{Bool}
end

function overlap(a::AbstractString, b::AbstractString)::Int
    len_a = length(a)
    len_b = length(b)
    max_len = min(len_a, len_b)

    for i in max_len:-1:1
        if a[len_a - i + 1:len_a] == b[1:i]
            return i
        end
    end
    return 0
end

# This function (primarily for initial load generation on master) now takes a common SolverState type
function generate_initial_load_get_subproblems(
    solver_state_obj::SolverState, # Takes a SolverState object
    current::String,
    used::Vector{Bool},
    level::Int,
    cutoff_level::Int,
    pool_subproblems::Vector{Subproblem}
)
    if level == cutoff_level
        push!(pool_subproblems, Subproblem(current, copy(used)))
        solver_state_obj.num_subproblems_generated += 1 
        return
    end

    for i in 1:solver_state_obj.num_reads
        if !used[i]
            used[i] = true

            solver_state_obj.num_overlap_verifications += 1
            ov = overlap(current, solver_state_obj.reads[i])

            temp_superstring = current * solver_state_obj.reads[i][ov+1:end]
            curr_len = length(temp_superstring)

            if curr_len < solver_state_obj.best_len 
                generate_initial_load_get_subproblems(
                    solver_state_obj, temp_superstring, used, level + 1, cutoff_level, pool_subproblems
                )
            end

            used[i] = false 
        end
    end
end


# This is the core function for workers to solve a subproblem
function solve_subproblem_on_worker(
    initial_subproblem::Subproblem,
    all_reads::Vector{String},
    num_all_reads::Int,
    cutoff_level::Int,
    current_global_best_len::UInt64 # Master sends its best knowledge for pruning
)
    # Each worker needs its own local SolverState for its search
    worker_solver_state = SolverState(all_reads, num_all_reads, current_global_best_len) # Uses the shared SolverState

    solve_build_superstring(
        worker_solver_state,
        initial_subproblem.current,
        copy(initial_subproblem.used), # Ensure a deep copy for independent search
        cutoff_level, 
        length(initial_subproblem.current)
    )
    
    # Return the worker's best result for the subproblem, and its counts
    return (worker_solver_state.best_len, 
            worker_solver_state.best_result, 
            worker_solver_state.num_solutions, 
            worker_solver_state.num_overlap_verifications)
end

# This is the recursive search function, called by solve_subproblem_on_worker
function solve_build_superstring(
    solver_state_obj::SolverState, # Takes the common SolverState type
    current::String,
    used::Vector{Bool},
    level::Int,
    curr_len::Int
)
    if curr_len >= solver_state_obj.best_len
        return 
    end

    if level == solver_state_obj.num_reads
        solver_state_obj.num_solutions += 1
        if curr_len < solver_state_obj.best_len
            solver_state_obj.best_len = UInt64(curr_len)
            solver_state_obj.best_result = current
        end
        return
    end

    for i in 1:solver_state_obj.num_reads
        if !used[i]
            used[i] = true

            solver_state_obj.num_overlap_verifications += 1
            ov = overlap(current, solver_state_obj.reads[i])

            temp_superstring = current * solver_state_obj.reads[i][ov+1:end]
            next_len = length(temp_superstring)

            if next_len < solver_state_obj.best_len
                solve_build_superstring(solver_state_obj, temp_superstring, used, level + 1, next_len)
            end

            used[i] = false 
        end
    end
end

end # module SuperstringWorkerLogic