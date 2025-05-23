using Printf
using Dates # For timing

# Define constants.
const MAX_READS = 20
const MAX_LEN = 100
const POOL_SIZE = 10000
const MAX_SUPERSTRING_LEN = MAX_READS * MAX_LEN 

mutable struct SolverState
    reads::Vector{String}
    num_reads::Int
    num_subproblems_generated::UInt32 # Corresponds to unsigned int
    num_solutions::UInt64
    num_overlap_verifications::UInt64
    best_len::UInt64 # Lengths are non-negative, so UInt64 is appropriate
    best_result::String

    # Constructor
    function SolverState()
        new([], 0, 0, 0, 0, typemax(UInt64), "")
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

function generate_initial_load_get_subproblems(solver_state::SolverState, current::String, used::Vector{Bool}, level::Int,
    cutoff_level::Int,pool_subproblems::Vector{Subproblem})

    
    if level == cutoff_level
        push!(pool_subproblems, Subproblem(current, copy(used)))
        solver_state.num_subproblems_generated += 1
        return
    end

    for i in 1:solver_state.num_reads
        if !used[i]
            used[i] = true

            solver_state.num_overlap_verifications += 1
            ov = overlap(current, solver_state.reads[i])

         
            temp_superstring = current * solver_state.reads[i][ov+1:end]
            curr_len = length(temp_superstring)

         
            if curr_len < solver_state.best_len
                generate_initial_load_get_subproblems(
                    solver_state, temp_superstring, used, level + 1, cutoff_level, pool_subproblems
                )
            end

            used[i] = false 
        end
    end
end

function solve_build_superstring(solver_state::SolverState,current::String,used::Vector{Bool},level::Int,curr_len::Int)

    if level == solver_state.num_reads
        solver_state.num_solutions += 1
        if curr_len < solver_state.best_len
            solver_state.best_len = UInt64(curr_len)
            solver_state.best_result = current
        end
        return
    end

    for i in 1:solver_state.num_reads
        if !used[i]
            used[i] = true

            solver_state.num_overlap_verifications += 1
            ov = overlap(current, solver_state.reads[i])

            temp_superstring = current * solver_state.reads[i][ov+1:end]
            next_len = length(temp_superstring)

            # Prune: if current length is already worse than best
            if next_len < solver_state.best_len
                solve_build_superstring(solver_state, temp_superstring, used, level + 1, next_len)
            end

            used[i] = false # Backtrack
        end
    end
end

function solve_launch_sequential_search(
    solver_state::SolverState,
    pool_of_subproblems::Vector{Subproblem},
    cutoff_level::Int
)
    for sub_problem in pool_of_subproblems
        solve_build_superstring(
            solver_state,
            sub_problem.current,
            sub_problem.used, # This is a copy from the pool, so modifications here are fine
            cutoff_level,
            length(sub_problem.current)
        )
    end
end

function julia_main(args::Vector{String})::Cint
    if length(args) != 2
        @error "Usage: julia <script_name>.jl reads.txt cutoff_level"
        return 1
    end

    filename = args[1]
    cutoff_level = parse(Int, args[2]) # Convert string argument to Int

    solver_state = SolverState() # Create an instance of our solver state

    # Read reads from file
    try
        open(filename, "r") do io
            for line in eachline(io)
                push!(solver_state.reads, strip(line))
            end
        end
    catch e
        @error "Error opening or reading file $filename: $e"
        return 1
    end

    solver_state.num_reads = length(solver_state.reads)

    println("\n############## Problem Read -- OK ##############")
    println("\nNum reads: $(solver_state.num_reads)")

    # Prepare for initial load generation
    initial_string = ""
    used = fill(false, solver_state.num_reads)
   
    pool_of_subproblems = Vector{Subproblem}()
    sizehint!(pool_of_subproblems, POOL_SIZE) 

    initial_load_start_time = now()
    for i in 1:solver_state.num_reads
        used[i] = true
        initial_string = solver_state.reads[i]
        generate_initial_load_get_subproblems(
            solver_state, initial_string, used, 1, cutoff_level, pool_of_subproblems
        )
        used[i] = false 
    end
    initial_load_end_time = now()

    println("\nCutoff depth: $(cutoff_level), Num subproblems: $(solver_state.num_subproblems_generated)")
    println("Initial Load Generation Time: $(canonicalize(Dates.Millisecond(initial_load_end_time - initial_load_start_time)))")

    # Start timer for solving subproblems
    solve_start_time = now()
    solve_launch_sequential_search(solver_state, pool_of_subproblems, cutoff_level)
    solve_end_time = now()


    println("\nBest superstring: $(solver_state.best_result)")
    println("Length: $(solver_state.best_len)")
    println("Number of stringcomp calls: $(solver_state.num_overlap_verifications)")
    println("Number of complete solutions found: $(solver_state.num_solutions)")
    println("Subproblem Solving Time: $(canonicalize(Dates.Millisecond(solve_end_time - solve_start_time)))")

    return 0
end

if abspath(PROGRAM_FILE) == @__FILE__
    julia_main(ARGS)
end