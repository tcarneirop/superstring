module HelloWorkerFunctions
using Distributed # Include if your worker functions use Distributed.jl primitives (e.g., myid(), @spawn)

function say_hello_from_worker()
    println("Worker says: Hello from worker! My ID is $(myid())")
    # You can put other worker-specific functions here
end

end # module HelloWorkerFunctions