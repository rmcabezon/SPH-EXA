#include <iostream>

#include "sphexa.hpp"
#include "SqPatch.hpp"

using namespace std;
using namespace sphexa;

template<typename T>
void reorderParticles(SqPatch<T> &d, std::vector<int> &ordering)
{
    d.reorder(ordering);
    for(unsigned i=0; i<d.x.size(); i++)
        ordering[i] = i;
}

int main()
{
    typedef double Real;
    typedef Octree<Real> Tree;
    //typedef HTree<Real> Tree;

    SqPatch<Real> d(1e6, "bigfiles/squarepatch3D_1M.bin");

    Tree tree(d.x, d.y, d.z, d.h, Tree::Params(/*max neighbors*/d.ngmax, /*bucketSize*/128));

    // Main computational tasks
    LambdaTask tComputeBBox([&](){ d.computeBBox(); });
    BuildTree<Tree, Real> tBuildTree(d.bbox, tree);
    LambdaTask tReorderParticles([&](){ reorderParticles(d, *tree.ordering); });
    FindNeighbors<Tree, Real> tFindNeighbors(tree, d.neighbors, d.h, FindNeighbors<Tree, Real>::Params(d.ngmin, d.ng0, d.ngmax));
    Density<Real> tDensity(d.x, d.y, d.z, d.h, d.m, d.neighbors, d.ro);
    EquationOfStateSquarePatch<Real> tEquationOfState(d.ro_0, d.p_0, d.iteration, d.p, d.u, d.ro, d.c);
    MomentumSquarePatch<Real> tMomentum(d.x, d.y, d.z, d.h, d.vx, d.vy, d.vz, d.ro, d.p, d.c, d.m, d.iteration, d.neighbors, d.grad_P_x, d.grad_P_y, d.grad_P_z);
    Energy<Real> tEnergy(d.x, d.y, d.z, d.h, d.vx, d.vy, d.vz, d.ro, d.p, d.c, d.m, d.neighbors, d.du);
    Timestep<Real> tTimestep(d.h, d.c, d.dt_m1, d.dt);
    UpdateQuantities<Real> tUpdateQuantities(d.grad_P_x, d.grad_P_y, d.grad_P_z, d.dt, d.du, d.iteration, d.bbox, d.x, d.y, d.z, d.vx, d.vy, d.vz, d.x_m1, d.y_m1, d.z_m1, d.u, d.du_m1, d.dt_m1);
    EnergyConservation<Real> tEnergyConservation(d.u, d.vx, d.vy, d.vz, d.m, d.etot, d.ecin, d.eint);
    
    // Small tasks to check the result
    LambdaTask tPrintBBox([&](){
        cout << "### Check ### Computational domain: ";
        cout << d.bbox.xmin << " " << d.bbox.xmax << " ";
        cout << d.bbox.ymin << " " << d.bbox.ymax << " ";
        cout << d.bbox.zmin << " " << d.bbox.zmax << endl;
    });
    LambdaTask tCheckNeighbors([&]()
    { 
        int sum = 0;
        for(unsigned int i=0; i<d.neighbors.size(); i++)
            sum += d.neighbors[i].size();
        cout << "### Check ### Avg. number of neighbours: " << sum/d.n << "/" << d.ng0 << endl;
    });
    LambdaTask tCheckTimestep([&](){ cout << "### Check ### Old Time-step: " << d.dt_m1[0] << ", New time-step: " << d.dt[0] << endl; });
    LambdaTask tCheckConservation([&](){ cout << "### Check ### Total energy: " << d.etot << ", (internal: " << d.eint << ", cinetic: " << d.ecin << ")" << endl; });
    LambdaTask twriteFile([&](){ d.writeFile(); });

    // We build the workflow
    Workflow work;
    work.add(&tComputeBBox);

    work.add(&tBuildTree, Workflow::Params(1, "Building Tree"));
    work.add(&tReorderParticles, Workflow::Params(1, "Reordering Particles"));

    work.add(&tFindNeighbors, Workflow::Params(1, "Finding Neighbors"));
    work.add(&tDensity, Workflow::Params(1, "Computing Density"));
    work.add(&tEquationOfState, Workflow::Params(1, "Computing Equation Of State"));
    work.add(&tMomentum, Workflow::Params(1, "Computing Momentum"));
    work.add(&tEnergy, Workflow::Params(1, "Computing Energy"));

    work.add(&tTimestep, Workflow::Params(1, "Updating Time-step"));
    work.add(&tUpdateQuantities, Workflow::Params(1, "Updating Quantities"));
    work.add(&tEnergyConservation, Workflow::Params(1, "Computng Total Energy"));

    work.add(&tPrintBBox);
    work.add(&tCheckNeighbors);
    work.add(&tCheckTimestep);
    work.add(&tCheckConservation);
    work.add(&twriteFile, Workflow::Params(1, "Writing File"));

    for(d.iteration = 0; d.iteration < 200; d.iteration++)
    {
        cout << "Iteration: " << d.iteration << endl;
        work.exec();
        cout << endl;
    }

    // int chunkSize = 500;
    // int nChunks = d.n/chunkSize;

    // for(d.iteration = 0; d.iteration < 64; d.iteration++)
    // {
    //     cout << "Iteration: " << d.iteration << endl;

    //     tComputeBBox.compute();
    //     tBuildTree.compute();
    //     tReorderParticles.compute();
        
    //     #pragma omp parallel for
    //     for(int chunk=0; chunk<nChunks; chunk++)
    //     {
    //         int start = chunk * chunkSize;
    //         int end = start + chunkSize;

    //         for(int i=start; i<end; i++)
    //         {
    //             tFindNeighbors.compute(i);
    //             tDensity.compute(i);
    //             tEquationOfState.compute(i);
    //         }
    //     }

    //     #pragma omp parallel for
    //     for(int chunk=0; chunk<nChunks; chunk++)
    //     {
    //         int start = chunk * chunkSize;
    //         int end = start + chunkSize;

    //         for(int i=start; i<end; i++)
    //         {
    //             // Need Equation of State to be computed for all neighbors of i
    //             tMomentum.compute(i);
    //             tEnergy.compute(i);
    //             tTimestep.compute(i);
    //         }
    //     }

    //     cout << "Chunks done" << endl;

    //     // REDUCTION: Update global Time-Step
    //     tTimestep.postProcess();

    //     // Need new time-step
    //     #pragma omp parallel for
    //     for(int i=0; i<d.n; i++)
    //         tUpdateQuantities.compute(i);
        
    //     // REDUCTION: sum total energy
    //     tEnergyConservation.compute();

    //     // Checks
    //     tPrintBBox.compute();
    //     tCheckNeighbors.compute();
    //     tCheckTimestep.compute();
    //     tCheckConservation.compute();

    //     //cout << "Writing file" << endl;
    //     //twriteFile.compute();

    //     cout << endl;
    // }

    return 0;
}



