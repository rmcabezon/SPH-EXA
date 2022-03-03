#pragma once

#include <tuple>

#include <omp.h>

namespace sphexa
{

auto initMpi()
{
    int rank     = 0;
    int numRanks = 0;
#ifdef USE_MPI
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        int mpi_version, mpi_subversion;
        MPI_Get_version(&mpi_version, &mpi_subversion);
        MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
#ifdef _OPENMP
        printf("# %d MPI-%d.%d process(es) with %d OpenMP-%u thread(s)/process\n",
               numRanks,
               mpi_version,
               mpi_subversion,
               omp_get_max_threads(),
               _OPENMP);
#else
        printf("# %d MPI-%d.%d process(es) with 1 OpenMP thread/process\n", mpi_ranks, mpi_version, mpi_subversion);
#endif
    }
#endif
    return std::make_tuple(rank, numRanks);
}

int exitSuccess()
{
#ifdef USE_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS;
}

} // namespace sphexa
