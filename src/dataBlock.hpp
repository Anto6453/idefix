#ifndef DATABLOCK_HPP
#define DATABLOCK_HPP
#include "idefix.hpp"

#define     BOUNDARY_

// Forward declaration of DataBlock
class DataBlock;
class ElectroMotiveForce {
    public:
        /* Face centered emf components */
        IdefixArray3D<real>     exj;
        IdefixArray3D<real>     exk;
        IdefixArray3D<real>     eyi;
        IdefixArray3D<real>     eyk;
        IdefixArray3D<real>     ezi;
        IdefixArray3D<real>     ezj;

        /* Edge centered emf components */
        IdefixArray3D<real>     ex;
        IdefixArray3D<real>     ey;
        IdefixArray3D<real>     ez;

        /* Range of existence */
        int  ibeg, jbeg, kbeg;
        int  iend, jend, kend;

        // Constructor from datablock structure
        ElectroMotiveForce(DataBlock *);

        // Default constructor
        ElectroMotiveForce();

};

class DataBlock {
public:
    // Local grid information
    IdefixArray1D<real> x[3];      // geometrical central points
    IdefixArray1D<real> xr[3];     // cell right interface
    IdefixArray1D<real> xl[3];     // cell left interface
    IdefixArray1D<real> dx[3];     // cell width

    real xbeg[3];                   // Beginning of datablocl
    real xend[3];                   // End of datablock

    IdefixArray3D<real> dV;     // cell volume
    IdefixArray3D<real> A[3];      // cell right interface area

    IdefixArray4D<real> Vc;     // Main cell-centered primitive variables index
    IdefixArray4D<real> Vs;     // Main face-centered varariables
    IdefixArray4D<real> Uc;     // Main cell-centered conservative variables

    // Required by time integrator
    IdefixArray4D<real> Vc0;
    IdefixArray4D<real> Vs0;
    IdefixArray3D<real> InvDtHyp;
    IdefixArray3D<real> InvDtPar;

    // Required by physics
    IdefixArray4D<real> PrimL;
    IdefixArray4D<real> PrimR;
    IdefixArray4D<real> FluxRiemann;

    // Name of the fields (used in outputs)
    std::vector<std::string> VcName;
    std::vector<std::string> VsName;
   
    int np_tot[3];                  // total number of grid points
    int np_int[3];                  // internal number of grid points

    int nghost[3];                  // number of ghost cells
    BoundaryType lbound[3];                  // Boundary condition to the left
    BoundaryType rbound[3];                  // Boundary condition to the right

    int beg[3];                     // Begining of internal indices
    int end[3];                     // End of internal indices

    int gbeg[3];                    // Begining of local block in the grid (internal)
    int gend[3];                    // End of local block in the grid (internal)

    ElectroMotiveForce emf;

    // init from a Grid object
    void InitFromGrid(Grid &);

    // MPI Exchange functions
    void ExchangeAll();
    void ExchangeX1();
    void ExchangeX2();
    void ExchangeX3();

    enum {faceRight, faceLeft};

    DataBlock();

private:
    Grid *mygrid;

    // Buffers for MPI calls
    IdefixArray1D<real> BufferSendX1[2];
    IdefixArray1D<real> BufferSendX2[2];
    IdefixArray1D<real> BufferSendX3[2];
    IdefixArray1D<real> BufferRecvX1[2];
    IdefixArray1D<real> BufferRecvX2[2];
    IdefixArray1D<real> BufferRecvX3[2];

    int bufferSizeX1;
    int bufferSizeX2;
    int bufferSizeX3;


};



#endif