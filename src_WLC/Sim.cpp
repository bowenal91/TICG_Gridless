#include "time.h"
#include <iostream>
#include <math.h>
#include <cstdlib>
#include <stdio.h>
#include <algorithm>
#include "Sim.hpp"
#include "mersenne.h"
#define PI 3.14159265358979323846


using namespace LAMMPS_NS;
using namespace std;

Sim::Sim() {
    read_input();
    read_configuration();
    read_topology();
    generate_lists();
    generate_matrices();
    shiftCOM();
    initialize_grid();
    initialize_data();
    openFiles();
}

void Sim::MCSim() {
    box.numAccepts = 0;
    box.numRejects = 0;
    for (box.cycle=0;box.cycle<box.numCycles;box.cycle++) {
        updateT();
        if (box.cycle%10 == 0) {
            moveGrid();
        }
        if (box.cycle%box.vizInterval == 0) {
            shiftCOM();
            printXYZ();
            dumpData();
            writeEnergy();
            printf("Turn: %d\n",box.cycle);
            //int i,j,k;
            /*
            FILE *q = fopen("Qtensor.dat","w");
            for (i=0;i<box.numGrid;i++) {
                fprintf(q,"%d\n",i);
                for (j=0;j<3;j++) {
                    for (k=0;k<3;k++) {
                        fprintf(q,"%f\t",grid[i].Q.val[j][k]);
                    }
                    fprintf(q,"\n");
                }
                fprintf(q,"\n");
            }
            */
        }
        MCMove();
        if (box.cycle%box.checkInterval == 0) {
            print_configuration();
        }
    }
    writeEnergy();
    shiftCOM();
    dumpData();
    printXYZ();
    //printPOV();
    printf("Percent Acceptance: %f\n",100*double(box.numAccepts)/double(box.numAccepts+box.numRejects));
    empty_containers();
}

void Sim::openFiles() {
    FILE *dump;
    dump = fopen("config.xyz","w");
    fclose(dump);
    dump=fopen("Energy.txt","w");
    fclose(dump);
    
    dump = fopen("data.txt","w");
    fprintf(dump,"%d\n",box.numBeads);
    fclose(dump);
    
    printPSF();

}

void Sim::read_input() {

  FILE *input;
  char tt[2001];
  int ret;
  input=fopen("input", "r");

  if (input!=NULL)
    { 	/******		Reading Input File		********/
      ret=fscanf(input,"%d", &box.seed);		     fgets(tt,2000,input);
      ret=fscanf(input,"%d", &box.checkpoint);		 fgets(tt,2000,input);
      ret=fscanf(input,"%d", &box.numCycles);		 fgets(tt,2000,input);
      ret=fscanf(input,"%d", &box.numEq);		 fgets(tt,2000,input);
      ret=fscanf(input,"%d", &box.numAnneal);		 fgets(tt,2000,input);
      ret=fscanf(input,"%d", &box.vizInterval);		 fgets(tt,2000,input);
      ret=fscanf(input,"%d", &box.checkInterval);    fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.fracDisplace);    fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.fracRotate);		 fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.fracTranslate);   fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.fracPivot);   fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.x);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.y);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.z);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.grid_dx);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.grid_dy);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.grid_dz);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.chi);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.kappa);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.mu);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.K);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf", &box.Delta);               fgets(tt,2000,input);
      ret=fscanf(input,"%d",  &box.hasWall);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf",  &box.displaceMax);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf",  &box.translateMax);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf",  &box.rotateMax);               fgets(tt,2000,input);
      ret=fscanf(input,"%lf",  &box.pivotMax);               fgets(tt,2000,input);
      fgets(tt,2000,input);fgets(tt,2000,input);

      fclose(input);
      /***************************************/
    }

  else
    { 
      fprintf(stdout,"\n Input File Not Found\n");
      exit(EXIT_FAILURE);
    }
    


}

void Sim::read_configuration() {
    int i,start,end,anchor,rigid,rigidBond;
    char tt[2001];
    

    FILE *atom_file = fopen("beads.dat","r"); //Location of all atom files 
    //fgets(tt,2000,atom_file);             //Section title
    fscanf(atom_file, "%d",&box.numBeads);
    fscanf(atom_file, "%d",&box.numTypes);
    beads = new Bead[box.numBeads];
    for (i=0;i<box.numBeads;i++) {
        fscanf(atom_file,"%d", &beads[i].id);
        fscanf(atom_file,"%lf%lf%lf", &beads[i].r[0], &beads[i].r[1], &beads[i].r[2]);
        fscanf(atom_file,"%lf%lf%lf", &beads[i].rn[0], &beads[i].rn[1], &beads[i].rn[2]);
        fscanf(atom_file,"%lf%lf%lf", &beads[i].u[0], &beads[i].u[1], &beads[i].u[2]);
        fscanf(atom_file,"%d", &beads[i].type);
        fscanf(atom_file,"%d", &start);
        fscanf(atom_file,"%d", &end);
        fscanf(atom_file,"%d", &anchor);
        fscanf(atom_file,"%d", &rigid);
        fscanf(atom_file,"%d", &rigidBond);
        if (start==1) {
            beads[i].isStart = true;
        } else {
            beads[i].isStart = false;
        }        
        if (end==1) {
            beads[i].isEnd = true;
        } else {
            beads[i].isEnd = false;
        }
        if (anchor==1) {
            beads[i].isAnchor = true;
        } else {
            beads[i].isAnchor = false;
        }
        if (rigid==1) {
            beads[i].isRod = true;
        } else {
            beads[i].isRod = false;
        }
        if (rigidBond==1) {
            beads[i].hasRigidBonds = true;
        } else {
            beads[i].hasRigidBonds = false;
        }

        PBC_shift(beads[i].r,beads[i].rn); //Makes it easier when writing initial configurations
    }
   
    for (i=0;i<box.numBeads;i++) {
        update_single_u_vector(i);
    }
    fclose(atom_file);
     
    box.dens = double(box.numBeads)/(box.x*box.y*box.z);
}

void Sim::read_topology() {
    int i;
    int start,end;
    char tt[2001];
    FILE *topo_file = fopen("topology.dat","r");

    fgets(tt,2000,topo_file);             //BONDS
    fscanf(topo_file,"%d",&box.numBonds);
    bonds = new Bond[box.numBonds];
    for (i=0;i<box.numBonds;i++) {
        fscanf(topo_file,"%d%d%lf%lf",&bonds[i].id1, &bonds[i].id2, &bonds[i].k_G, &bonds[i].r0);
    }
    
    fgets(tt,2000,topo_file);             //ANGLES
    fgets(tt,2000,topo_file);             //ANGLES
    fscanf(topo_file,"%d",&box.numAngles);
    angles = new Angle[box.numAngles];
    for (i=0;i<box.numAngles;i++) {
        fscanf(topo_file,"%d%d%d%lf",&angles[i].id0,&angles[i].id1,&angles[i].id2,&angles[i].k_b);
    }

    fgets(tt,2000,topo_file);             //MOLECULES
    fgets(tt,2000,topo_file);             //MOLECULES
    fscanf(topo_file,"%d",&box.numMolecules);
    moleculeList.clear();
    moleculeList.resize(box.numMolecules);
    for (i=0;i<box.numMolecules;i++) {
        fscanf(topo_file,"%d%d",&start,&end); //Start and end for atoms
        moleculeList[i].push_back(start);
        moleculeList[i].push_back(end);
        fscanf(topo_file,"%d%d",&start,&end); //Start and end for bonds
        moleculeList[i].push_back(start);
        moleculeList[i].push_back(end);
        fscanf(topo_file,"%d%d",&start,&end); //Start and end for angles
        moleculeList[i].push_back(start);
        moleculeList[i].push_back(end);        
    }

    fclose(topo_file);
}

void Sim::print_configuration() {
    int i;
    char tt[2001];
    int start,end,anchor,rigid,rigidBond;
    FILE *atom_file = fopen("beads.dat","w"); //Location of all atom files 
    fprintf(atom_file, "%d\n",box.numBeads);
    fprintf(atom_file, "%d\n",box.numTypes);
    for (i=0;i<box.numBeads;i++) {
        fprintf(atom_file,"%d\t", beads[i].id);
        fprintf(atom_file,"%lf\t%lf\t%lf\t", beads[i].r[0], beads[i].r[1], beads[i].r[2]);
        fprintf(atom_file,"%lf\t%lf\t%lf\t", beads[i].rn[0], beads[i].rn[1], beads[i].rn[2]);
        fprintf(atom_file,"%lf\t%lf\t%lf\t", beads[i].u[0], beads[i].u[1], beads[i].u[2]);
        if (beads[i].isStart) {
            start = 1;
        } else {
            start = 0;
        }
        if (beads[i].isEnd) {
            end = 1;
        } else {
            end = 0;
        } if (beads[i].isAnchor) {
            anchor = 1;
        } else {
            anchor = 0;
        }
        if (beads[i].isRod) {
            rigid = 1;
        } else {
            rigid = 0;
        }
        if (beads[i].hasRigidBonds) {
            rigidBond = 1;
        } else {
            rigidBond = 0;
        }
        fprintf(atom_file,"%d\t", beads[i].type);
        fprintf(atom_file,"%d\t", start);
        fprintf(atom_file,"%d\t", end);
        fprintf(atom_file,"%d\t", anchor);
        fprintf(atom_file,"%d\t", rigid);
        fprintf(atom_file,"%d\n", rigidBond);
        vec_norm(beads[i].u);
    }

    fclose(atom_file);
}

int Sim::mapToGrid(LAMMPS_NS::vector r) {
    double x,y,z,x1,y1,z1;
    int a,b,c,m;
    x = r[0];
    y = r[1];
    z = r[2];


    a = floor((x-box.grid_min_x)/box.grid_dx);
    b = floor((y-box.grid_min_y)/box.grid_dy);
    c = floor((z-box.grid_min_z)/box.grid_dz);

    a -= box.grid_num_x*int(floor(double(a)/double(box.grid_num_x))); 
    b -= box.grid_num_y*int(floor(double(b)/double(box.grid_num_y))); 
    c -= box.grid_num_z*int(floor(double(c)/double(box.grid_num_z))); 
    
    m = c*box.grid_num_x*box.grid_num_y + b*box.grid_num_x + a;

    return m;
}

void Sim::initialize_grid() {
    int i,j,k;
    box.grid_num_x = box.x / box.grid_dx;
    box.grid_num_y = box.y / box.grid_dy;
    box.grid_num_z = box.z / box.grid_dz;

    box.grid_dx = box.x / double(box.grid_num_x);
    box.grid_dy = box.y / double(box.grid_num_y);
    box.grid_dz = box.z / double(box.grid_num_z);

    //printf("%f\t%f\t%f\n",box.grid_dx,box.grid_dy,box.grid_dz);

    if(box.hasWall) {
        box.grid_num_z += 2;
        
        box.grid_min_x = 0.0;
        box.grid_min_y = 0.0;
        box.grid_min_z = -box.grid_dz;
    } else {
        box.grid_min_x = 0.0;
        box.grid_min_y = 0.0;
        box.grid_min_z = 0.0;
    }

    box.numGrid = box.grid_num_x*box.grid_num_y*box.grid_num_z;

    grid = new Grid[box.numGrid];
    for (i=0;i<box.numGrid;i++) {
        for (j=0;j<box.numTypes;j++) {
            grid[i].phi.push_back(0.0);
        }
    }
    calc_grid_data();
    meshBeads();

}

void Sim::moveGrid() { //Randomly displace the grid and re-mesh all the beads
    int i,j;
    for (i=0;i<box.numGrid;i++) {
        for (j=0;j<box.numTypes;j++) {
            grid[i].phi[j] = 0.0;
        }
        //grid[i].Q.clear();
        grid[i].bead_list.clear();
    }

    box.grid_min_x += (2*RanGen->Random()-1)*box.grid_dx; 
    box.grid_min_y += (2*RanGen->Random()-1)*box.grid_dy; 
    box.grid_min_z += (2*RanGen->Random()-1)*box.grid_dz;

    box.grid_min_x -= box.x*floor(box.grid_min_x/box.x);
    box.grid_min_y -= box.y*floor(box.grid_min_y/box.y);
    if (box.hasWall) {
        box.grid_min_z -= (box.z+2*box.grid_dz)*floor((box.grid_min_z+box.grid_dz)/(box.z+2*box.grid_dz));  
    } else {
        box.grid_min_z -= box.z*floor(box.grid_min_z/box.z);
    }


    calc_grid_data();
    meshBeads();
    box.E_tot = calc_total_energy();
}

void Sim::calc_grid_data() { //Initialize all the bounds of the grid. Deal with the extra stuff that comes with having a hard wall 
    int i,j,k,m;
    double max_x,max_y,max_z,min_x,min_y,min_z;
    double x,y,z,vol,frac,dens;
    vol = box.grid_dx * box.grid_dy * box.grid_dz;
    dens = box.dens;

    if (box.hasWall) {
        min_x = 0.0;
        min_y = 0.0;
        min_z = -box.grid_dz;

        max_x = box.x;
        max_y = box.y;
        max_z = box.z+box.grid_dz;
    } else {
        min_x = 0.0;
        min_y = 0.0;
        min_z = 0.0;

        max_x = box.x;
        max_y = box.y;
        max_z = box.z;
    }

    for (k=0;k<box.grid_num_z;k++) {
        for (j=0;j<box.grid_num_y;j++) {
            for (i=0;i<box.grid_num_x;i++) {
                m = k*box.grid_num_x*box.grid_num_y+j*box.grid_num_x+i;
                grid[m].id = m;

                x = i*box.grid_dx + box.grid_min_x;
                y = j*box.grid_dy + box.grid_min_y;
                z = k*box.grid_dz + box.grid_min_z;
                x -= (max_x-min_x)*floor( (x-min_x) / (max_x-min_x) );
                y -= (max_y-min_y)*floor( (y-min_y) / (max_y-min_y) );
                if (box.hasWall) {
                    z -= (max_z-min_z)*floor( (z-min_z)/(max_z-min_z));
                } else {
                    z -= (max_z-min_z)*floor( (z-min_z) / (max_z-min_z) );
                }

                grid[m].x = x;
                grid[m].y = y;
                grid[m].z = z;
     
                if (box.hasWall) {
                    if (z < -1.0*box.grid_dz || z > box.z) {
                        frac = 0.0;
                        grid[m].vol = 0.0;
                    }
                    else if (z < 0.0) {
                        frac = (box.grid_dz + z)/box.grid_dz; //fraction within the simulation box
                        grid[m].vol = vol*frac;
                    } else if (z > box.z-box.grid_dz) {
                        frac = (box.z - z)/box.grid_dz;
                        grid[m].vol = vol*frac;
                    } else {
                        frac = 1.0;
                        grid[m].vol = vol;
                    }
                } else {
                    frac = 1.0;
                    grid[m].vol = vol;
                }
                if (grid[m].vol == 0.0) {
                    grid[m].inc = 0.0;
                    //grid[m].Q.invdens = 0.0;
                } else {
                    grid[m].inc = 1.0/(dens*grid[m].vol);       //CHECK THIS LATER
                    //grid[m].Q.invdens = 1.0/(grid[m].vol*dens); //CHECK THIS LATER
                }

                //printf("%f\n",grid[m].vol);
                
                

            }
        }
    }
}

void Sim::meshBeads() {  //Take all beads in the simulation and put their densities and orientations into the appropriate grid location
    int i,m;
    for (i=0;i<box.numBeads;i++) {
        addBeadToGrid(i);
    }
}

void Sim::addBeadToGrid(int i) { //Add a bead into its corresponding grid location
    int m = mapToGrid(beads[i].r);
    grid[m].phi[beads[i].type] += grid[m].inc;
    if (beads[i].isRod) {
        grid[m].bead_list.push_back(i);
    }
    beads[i].grid = m;
}

void Sim::removeBeadFromGrid(int i) { //Remove a bead from its correspnding grid location
    int j;
    int m = mapToGrid(beads[i].r);
    grid[m].phi[beads[i].type] -= grid[m].inc;
    if (beads[i].isRod) {
        grid[m].bead_list.erase( std::remove( grid[m].bead_list.begin(), grid[m].bead_list.end(), i), grid[m].bead_list.end() );
    }
    beads[i].grid = -1; //Use -1 to mean it is currently not assigned to a grid
}

void Sim::generate_lists() { //Generate the lists containing all the bonds/angles/twists associated with each atom
    int i;
    
    bondList.clear();
    angleList.clear();
    
    bondList.resize(box.numBeads);
    angleList.resize(box.numBeads);

    for (i=0;i<box.numBonds;i++) { //Element N of bondList contains a list of id's for all bonds that atom N is a part of
        bondList[bonds[i].id1].push_back(i);
        bondList[bonds[i].id2].push_back(i);
    }

    for (i=0;i<box.numAngles;i++) {
        angleList[angles[i].id0].push_back(i);
        angleList[angles[i].id1].push_back(i);
        angleList[angles[i].id2].push_back(i);
    }

}


void Sim::generate_matrices() {
    int i,j;
    
    chis = new double *[box.numTypes];
    mus = new double *[box.numTypes];

    for (i=0;i<box.numTypes;i++) {
        
        chis[i] = new double [box.numTypes];
        mus[i] = new double [box.numTypes];

    }
   
    FILE *data = fopen("interactions.dat","r");
    for (i=0;i<box.numTypes;i++) {
        for (j=0;j<box.numTypes;j++) {
            fscanf(data,"%lf",&chis[i][j]);
        }
    }
      for (i=0;i<box.numTypes;i++) {
        for (j=0;j<box.numTypes;j++) {
            fscanf(data,"%lf",&mus[i][j]);
        }
    }

    fclose(data);

}


void Sim::initialize_data() {
    if (box.seed < 0) {
        box.seed = time(NULL);
    }
    RanGen = new CRandomMersenne(box.seed);
    box.E_tot = calc_total_energy(); 
}

void Sim::calc_random_vector(LAMMPS_NS::vector &b) {
    double R1,R2,R3;
    do { //Generate random unit vector and make it u.
        R1 = (2*RanGen->Random()-1);
        R2 = (2*RanGen->Random()-1);
        R3 = R1*R1+R2*R2;
    } while (R3>=1);
               
    b[0] = 2*sqrtl(1.0-R3)*R1;
    b[1] = 2*sqrtl(1.0-R3)*R2;
    b[2] = 1-2.0*R3;


}

void Sim::calc_random_normal_vector(LAMMPS_NS::vector &n, LAMMPS_NS::vector a) { //Uses a rotation matrix to calculate a unit vector, n, normal to input vector a
    double theta, phi, A,B,C,D,R1,x0,y0;
    double ax,ay,az;
    ax = a[0]; ay = a[1]; az = a[2];
    theta = acos(az / sqrt(ax*ax+ay*ay+az*az));
    phi = atan2(ay,ax);
    A = cos(theta);
    B = sin(theta);
    C = cos(phi);
    D = sin(phi);
    R1 = 2*PI*RanGen->Random();
    x0 = cos(R1);
    y0 = sin(R1);
    n[0] = A*C*x0 - D*y0;
    n[1] = A*D*x0 + C*y0;
    n[2] = -1*B*x0;

    
}

void Sim::calc_cross_vector(LAMMPS_NS::vector &c, LAMMPS_NS::vector a, LAMMPS_NS::vector b) {//Cross product
    
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];

}

void Sim::updateT() {
    /*
    int i = box.cycle;
    double dT = (box.T_anneal-box.T_fin)/double(box.numAnneal);
    int numSteps;
    if (i < box.numEq) {
        box.T = box.T_anneal;
    } else if (i > box.numEq && i < box.numEq+box.numAnneal) {
        numSteps = i - box.numEq;
        box.T = box.T_anneal - numSteps*dT;
    } else {
        box.T = box.T_fin;
    }
    */
}

/*********************************ENERGY CALCULATIONS**************************************/
double Sim::calc_total_energy() {
    int i,j;
    double E = 0.0;
    double grid,bond,angle; 
    for (i=0;i<box.numGrid;i++) {
        E += calc_grid_energy(i);
    }
    //grid = E;
    for (i=0;i<box.numBonds;i++) {
        E += calc_bond_energy(i);
    }
    //bond = E - grid;
    for (i=0;i<box.numAngles;i++) {
        E += calc_angle_energy(i);
    }
    //angle = E - grid - bond;
    //printf("%f\t%f\t%f\n",grid/double(box.numBeads),bond/double(box.numBonds),angle/double(box.numAngles));
    return E;
    
}

double Sim::calc_bond_energy(int i) {
    double r,k_G,r0,E;
    int id1,id2;
    int j;

    k_G = bonds[i].k_G; 
    r0 = bonds[i].r0;
    id1 = bonds[i].id1;
    id2 = bonds[i].id2;
    r = calc_dist(beads[id1].rn,beads[id2].rn);

    E = k_G*(r-r0)*(r-r0);

    return E;
}

double Sim::calc_angle_energy(int i) { //Calculate the energy associated with angle i (using cos(theta) because that is what wlc form reduces to)
    double k_b,E;
    int id0,id1,id2;
    
    k_b = angles[i].k_b;
    id0 = angles[i].id0;
    id1 = angles[i].id1;
    id2 = angles[i].id2;

    E = k_b*(1.0 + calc_cos_angle(beads[id0].rn,beads[id1].rn,beads[id2].rn));
    
    return E;

}

double Sim::calc_bead_energy(int i) { //Calculate all non-grid energy associated with a particular bead - this energy is in no way dependent on the u orientation vector
    int j,k;
    double E = 0.0;
    for (j=0;j<bondList[i].size();j++) {
        k = bondList[i][j];
        E += calc_bond_energy(k);
    }

    for (j=0;j<angleList[i].size();j++) {
        k = angleList[i][j];
        E += calc_angle_energy(k);
    }

    return E;
}

double Sim::calc_grid_energy(int i) { //Calculate all energy associated with a particular grid - NEED TO UPDATE THIS WHEN CONSIDERING MORE THAN BINARY INTERACTIONS
    double E,E1,E2;
    double vol  = grid[i].vol;
    double phi_tot = 0.0;
    int t1,t2;
    for (t1=0;t1<box.numTypes;t1++) {
        phi_tot += grid[i].phi[t1];
    }

    E1 = 0.5*box.kappa*phi_tot*phi_tot;
    
    for (t1=0;t1<box.numTypes;t1++) {
        for (t2=t1+1;t2<box.numTypes;t2++) {
            E1 += chis[t1][t2]*grid[i].phi[t1]*grid[i].phi[t2];
        }
    }

    E1 *= box.dens*vol*box.Delta;

    E2 = 0.0; 
    int j,k;
    //printf("%d\n",grid[i].bead_list.size());
    for (j=0;j<grid[i].bead_list.size();j++) {
        for (k=j+1;k<grid[i].bead_list.size();k++) {
            E2 += calc_nematic_pair_energy(grid[i].bead_list[j],grid[i].bead_list[k]);
        }
    }
    if (vol == 0.0) {
        E2 = 0.0;
    } else {
        E2 *= box.Delta/(box.dens*vol);
    }
    E = E1+E2;
    //printf("%f\t%f\t%f\t%f\t%f\n",phiA,phiB,E1,E2,E);

    return E;
}

double Sim::calc_nematic_pair_energy(int i, int j) {
    double dot1, dot2, E;
    int t1,t2;
    //LAMMPS_NS::vector rij, cross;


    //nearest_image_dist(rij, beads[i].r, beads[j].r);
    //vec_norm(rij);
    dot1 = vec_dot(beads[i].u, beads[j].u);
    //calc_cross_vector(cross, beads[i].u, beads[j].u);
    //dot2 = vec_dot(cross, rij);

    //E = -box.mu * dot1*dot1 + box.K*dot2*dot1;
    

    t1 = beads[i].type;
    t2 = beads[j].type;
    
    E = -mus[t1][t2] * dot1*dot1;

    return E;
    
}

double Sim::calc_molecule_energy(int m) { //Need to figure this crap out later on - MIGHT NOT BE PARTICULARLY USEFUL FOR GRID ENERGY MODEL
    double E = 0.0;
    int i,j,k;
    int start_atom,end_atom,start_bond,end_bond,start_angle,end_angle,start_twist,end_twist;

    start_atom = moleculeList[m][0];
    end_atom = moleculeList[m][1];
    start_bond = moleculeList[m][2];
    end_bond = moleculeList[m][3];
    start_angle = moleculeList[m][4];
    end_angle = moleculeList[m][5];

    if (end_bond-start_bond != 0) { 
        for (i=start_bond;i<=end_bond;i++) {
            E += calc_bond_energy(i);
        }
    }

    if (end_angle-start_angle != 0) {
        for (i=start_angle;i<=end_angle;i++) {
            E += calc_angle_energy(i);
        }
    }
    
    return E;

}

/*************************************************************************************************/

void Sim::update_u_vectors(int i) { //Update all u-vectors that are dependent on a bead's position: i, i-1, i+1 
    
    int j;

    if (beads[i].isStart) { //Bead is the start of a chain - forward difference
        
        update_single_u_vector(i);
        update_single_u_vector(i+1);

    } else if (beads[i].isEnd) { //Bead is end of a chain - backward difference

        update_single_u_vector(i);
        update_single_u_vector(i-1);

    } else { //Bead is in the middle of a chain - central difference

        update_single_u_vector(i+1);
        update_single_u_vector(i-1);

    }

}

void Sim::update_single_u_vector(int i) {   //Defines the u vector as the central difference of the two nearest connecting beads
    int j;

    if (beads[i].isStart) { //Bead is the start of a chain - forward difference

        for (j=0;j<3;j++) {
            beads[i].u[j] = beads[i+1].rn[j] - beads[i].rn[j];
        }

        vec_norm(beads[i].u);

    } else if (beads[i].isEnd) { //Bead is end of a chain - backward difference

        for (j=0;j<3;j++) {
            beads[i].u[j] = beads[i].rn[j] - beads[i-1].rn[j];
        }

        vec_norm(beads[i].u);

    } else { //Bead is in the middle of a chain - central difference

        for (j=0;j<3;j++) {
            beads[i].u[j] = beads[i+1].rn[j] - beads[i-1].rn[j];
        }

        vec_norm(beads[i].u);

    }

}

double Sim::calc_cos_angle(LAMMPS_NS::vector r0, LAMMPS_NS::vector r1, LAMMPS_NS::vector r2) { //Calculate and return the angle between vectors r2-r0 and r1-r0
    double costheta;
    LAMMPS_NS::vector v1,v2;
    v1[0] = r1[0] - r0[0]; 
    v1[1] = r1[1] - r0[1]; 
    v1[2] = r1[2] - r0[2]; 
    v2[0] = r2[0] - r0[0]; 
    v2[1] = r2[1] - r0[1]; 
    v2[2] = r2[2] - r0[2]; 

    vec_norm(v1);
    vec_norm(v2);

    costheta = vec_dot(v1,v2);
    if (costheta > 1.0) costheta = 1.0;
    if (costheta < -1.0) costheta = -1.0;
    return costheta;

}

double Sim::calc_dist(LAMMPS_NS::vector r1, LAMMPS_NS::vector r2) {

    double x,y,z,r;
    x = r1[0] - r2[0];
    y = r1[1] - r2[1];
    z = r1[2] - r2[2];

    r = sqrt(x*x+y*y+z*z);
    return r;

}

void Sim::nearest_image_dist(LAMMPS_NS::vector &r, LAMMPS_NS::vector r1, LAMMPS_NS::vector r2) { //r1 - r2 using the nearest image convention
    
    double dx,dy,dz;
    double xhalf = box.x/2;
    double yhalf = box.y/2;
    double zhalf = box.z/2;

    dx = r1[0] - r2[0];
    dy = r1[1] - r2[1];
    dz = r1[2] - r2[2];

    dx = dx > xhalf ? dx - box.x : dx; 
    dx = dx < -xhalf ? dx + box.x : dx; 
    dy = dy > yhalf ? dy - box.y : dy; 
    dy = dy < -yhalf ? dy + box.y : dy; 
    dz = dz > zhalf ? dz - box.z : dz; 
    dz = dz < -zhalf ? dz + box.z : dz; 

    r[0] = dx; r[1] = dy; r[2] = dz;

}

void Sim::PBC_shift(LAMMPS_NS::vector &r_shift, LAMMPS_NS::vector r) {
    r_shift[0] = r[0] - box.x*floor(r[0]/box.x);
    r_shift[1] = r[1] - box.y*floor(r[1]/box.y);
    r_shift[2] = r[2] - box.z*floor(r[2]/box.z);
}

void Sim::proj_vector(LAMMPS_NS::vector &a, LAMMPS_NS::vector n) {//Project vector a onto the plane described by unit normal vector n, then normalize the new a
    double dot;
    int i;
    LAMMPS_NS::vector n2;
    for (i=0;i<3;i++) {
        n2[i] = n[i];
    }
    dot = vec_dot(a,n2);
    for (i=0;i<3;i++) {
        a[i] = a[i] - dot*n2[i];
    }
    vec_norm(a);
}

/***************************************************MONTE CARLO MOVES******************************************/

void Sim::MCMove() {
    double displace = box.fracDisplace;
    double rotate = box.fracDisplace+box.fracRotate;
    double translate = box.fracDisplace+box.fracRotate+box.fracTranslate;
    double pivot = box.fracDisplace+box.fracRotate+box.fracTranslate+box.fracPivot;
   
    double rand = RanGen->Random();

    if (rand < displace) 
        MC_displace();
    else if (rand < rotate) 
        MC_rotate_molecule();
    else if (rand < translate) 
        MC_translate_molecule();
    else if (rand < pivot)
        MC_pivot();
}

//Philosophy: Get indices of all grids affected by the move before making the move. Do not double count affected grids. Need to watch grids that are affected by orientation changes

void Sim::MC_displace() { //Randomly displace all beads in the simulation
    int i,j,k,first,last,m,m2;
    double rx,ry,rz;
    double delta = box.displaceMax; //Maximum displacement distance in any direction
    double pre_energy, post_energy, dE;
    LAMMPS_NS::vector rnew,rper;
    Bead *save = new Bead[3];
    Grid *save_grid = new Grid[4]; //Maximum number of grids that can change are 4 since two of the beads don't actually move
    std::vector<int> grid_ids; //ID's of grids affected by the monte carlo move
    double a,b;
    bool flag;
    
    for (int move=0;move<box.numBeads;move++) {

        do {
            i = double(box.numBeads)*RanGen->Random();
        } while(i >= box.numBeads);
       
        if (beads[i].isAnchor) continue; //If a bead is attached to a wall then it will not be moved
        if (beads[i].hasRigidBonds) continue; //If the bead has rigid bonds, then a displace move will not be accepted

        m = beads[i].grid;

        //Figure out which beads and grids are affected and store that data

        rx = (2.0*RanGen->Random()-1.0)*delta;
        ry = (2.0*RanGen->Random()-1.0)*delta;
        rz = (2.0*RanGen->Random()-1.0)*delta;

        save[0] = beads[i];
        grid_ids.push_back(beads[i].grid);
        if (!beads[i].isStart) {
            save[1] = beads[i-1];
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == beads[i-1].grid) flag = false;
            }
            if (flag) grid_ids.push_back(beads[i-1].grid); //Prior bead's grid is included if it has not already been counted
        }
        if(!beads[i].isEnd) {
            save[2] = beads[i+1];
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == beads[i+1].grid) flag = false;
            }
            if (flag) grid_ids.push_back(beads[i+1].grid); //Following bead's grid is included if it has not already been counted
        }

        rnew[0] = beads[i].rn[0] + rx;
        rnew[1] = beads[i].rn[1] + ry;
        rnew[2] = beads[i].rn[2] + rz;
     
        if (box.hasWall) {
            if(rnew[2] > box.z || rnew[2] < 0.0) {
                grid_ids.clear();
                continue; //Automatically reject any moves that push it through a wall
            }
        }

        PBC_shift(rper, rnew);  //Gotta figure out what to do with attached sites

        m2 = mapToGrid(rper);
        flag = true;
        for (j=0;j<grid_ids.size();j++) {
            if (grid_ids[j] == m2) flag = false;
        }
        if (flag) grid_ids.push_back(m2);   //New grid location is included if it has not already been counted
        
        pre_energy = calc_bead_energy(i);
        for (j=0;j<grid_ids.size();j++) {
            save_grid[j] = grid[grid_ids[j]];
            pre_energy += calc_grid_energy(grid_ids[j]);
        }
        
        //Data storage and energy calculations complete - now perform move
        removeBeadFromGrid(i);
        if (!beads[i].isStart) {
            removeBeadFromGrid(i-1);
        }
        if (!beads[i].isEnd) {
            removeBeadFromGrid(i+1);
        }

        for (j=0;j<3;j++) {
            beads[i].rn[j] = rnew[j];
            beads[i].r[j] = rper[j];
        }
        update_u_vectors(i);

        addBeadToGrid(i);
        if (!beads[i].isStart) {
            addBeadToGrid(i-1);
        }
        if (!beads[i].isEnd) {
            addBeadToGrid(i+1);
        }

        //Move complete now perform post move energy calculations
        post_energy = calc_bead_energy(i);
        for (j=0;j<grid_ids.size();j++) {
            post_energy += calc_grid_energy(grid_ids[j]);
        }
        
        dE = post_energy - pre_energy;
        
        if (RanGen->Random() < exp(-dE)) { //Accept changes
            box.E_tot += dE;
            box.numAccepts++;
        } else { //Reverse changes
            //Restore Beads
            beads[i] = save[0];
            if (!beads[i].isStart) {
                beads[i-1] = save[1];
            }
            if (!beads[i].isEnd) {
                beads[i+1] = save[2];
            }
            //Restore Grids
            for (j=0;j<grid_ids.size();j++) {
                grid[grid_ids[j]] = save_grid[j];
            }
            
            box.numRejects++;
        }
        
        grid_ids.clear();
        
    }
    
    delete [] save;
    delete [] save_grid;
}

void Sim::MC_translate_molecule() { //Randomly translates an entire molecule -- however that is defined
    
    int moly,i,j,k,dir,core,disk_length,ref,first,last,tot_move,nAtoms;
    double x,y,z,xCenter,yCenter,zCenter;   
    double pre_energy, post_energy, dE;
    LAMMPS_NS::vector dist1; // Contains distance vector for each disk from center of mass for rotation
    LAMMPS_NS::vector dist2; // Contains new distance vector for each disk from center of mass for rotation
    double max_dist = box.translateMax;
    std::vector<int> grid_ids; //ID's of grids affected by the monte carlo move
    double a,b;
    bool flag,noWallBreak;


    for (int move=0;move<box.numMolecules;move++) {
        
        noWallBreak = true;

        do {
            moly = double(box.numMolecules)*RanGen->Random();
        } while(moly >= box.numMolecules);
        
        first = moleculeList[moly][0];
        last = moleculeList[moly][1];
        nAtoms = last-first+1;

        Bead *save = new Bead[nAtoms];
        Bead *update_chain = new Bead[nAtoms];
        Grid *save_grid = new Grid[2*nAtoms]; //Maximum number of grids affected are nAtoms for initial grids, and nAtoms for final grids

        for (i=0;i<nAtoms;i++) {
            save[i] = beads[first+i];
            update_chain[i] = beads[first+i];
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == save[i].grid) flag = false;
            }
            if (flag) grid_ids.push_back(save[i].grid);

            if (beads[first+i].isAnchor) noWallBreak = false;
        }
       
        x = max_dist*(2*RanGen->Random()-1.0); //Calculation for new center of mass
        y = max_dist*(2*RanGen->Random()-1.0); //Calculation for new center of mass
        z = max_dist*(2*RanGen->Random()-1.0); //Calculation for new center of mass
        
        
        for (j=0;j<nAtoms;j++) {
            update_chain[j].rn[0] += x;
            update_chain[j].rn[1] += y;
            update_chain[j].rn[2] += z;
            PBC_shift(update_chain[j].r,update_chain[j].rn);
            update_chain[j].grid = mapToGrid(update_chain[j].r);
            if ((update_chain[j].rn[2] < 0.0 || update_chain[j].rn[2] > box.z) && box.hasWall) {
                noWallBreak = false;
            }
            flag = true;
            for (i=0;i<grid_ids.size();i++) {
                if (grid_ids[i] == update_chain[j].grid) flag = false;
            }
            if (flag) grid_ids.push_back(update_chain[j].grid);
    

        }

        if (noWallBreak) {
            //Save any grids that will be affected by the move
            pre_energy = 0.0;
            for (i=0;i<grid_ids.size();i++) {
                save_grid[i] = grid[grid_ids[i]];
                pre_energy += calc_grid_energy(grid_ids[i]);
            }

            for (i=0;i<nAtoms;i++) {
                removeBeadFromGrid(first+i);
                beads[first+i] = update_chain[i];
                addBeadToGrid(first+i);
            }

            post_energy = 0.0;
            for (i=0;i<grid_ids.size();i++) {
                post_energy += calc_grid_energy(grid_ids[i]);
            }

            dE = post_energy - pre_energy;
        }
        if (RanGen->Random() < exp(-dE) && noWallBreak) { //Accept changes
            box.E_tot += dE; 
            box.numAccepts++;
        } else { 
            //Restore Beads
            for (i=0;i<nAtoms;i++) {
                beads[first+i] = save[i];
            }
            //Restore Grids
            if (noWallBreak) {
                for (i=0;i<grid_ids.size();i++) {
                    grid[grid_ids[i]] = save_grid[i];
                }
            }
            box.numRejects++;
        }


        delete [] save;
        delete [] update_chain;
        delete [] save_grid;

        grid_ids.clear();
    }
    

}

void Sim::MC_rotate_molecule() { //Randomly rotate an entire molecule around its center of mass

    int moly,i,j,k,dir,core,disk_length,ref,first,last,tot_move,nAtoms;
    double x,y,z,xCenter,yCenter,zCenter,theta;   
    double pre_energy, post_energy, dE;
    LAMMPS_NS::vector dist1; // Contains distance vector for each disk from center of mass for rotation
    LAMMPS_NS::vector dist2; // Contains new distance vector for each disk from center of mass for rotation
    LAMMPS_NS::vector axis; // Contains new distance vector for each disk from center of mass for rotation
    quaternion quat;
    double max_theta = box.rotateMax;
    std::vector<int> grid_ids; //ID's of grids affected by the monte carlo move
    double a,b;
    bool flag,noWallBreak;


    for (int move=0;move<box.numMolecules;move++) {
        
        noWallBreak = true;

        do {
            moly = double(box.numMolecules)*RanGen->Random();
        } while(moly >= box.numMolecules);
        
        first = moleculeList[moly][0];
        last = moleculeList[moly][1];
        nAtoms = last-first+1;

        Bead *save = new Bead[nAtoms];
        Bead *update_chain = new Bead[nAtoms];
        Grid *save_grid = new Grid[2*nAtoms]; //Maximum number of grids affected are nAtoms for initial grids, and nAtoms for final grids

        for (i=0;i<nAtoms;i++) {
            save[i] = beads[first+i];
            update_chain[i] = beads[first+i];
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == save[i].grid) flag = false;
            }
            if (flag) grid_ids.push_back(save[i].grid);

            if(beads[first+i].isAnchor) noWallBreak = false;
        }
       
        xCenter = 0.0;
        yCenter = 0.0;
        zCenter = 0.0;
        for (j=first;j<=last;j++) {
            xCenter += beads[j].rn[0];
            yCenter += beads[j].rn[1];
            zCenter += beads[j].rn[2];
        }
        xCenter /= double(nAtoms); //Calculation of center of mass of polymer
        yCenter /= double(nAtoms);
        zCenter /= double(nAtoms);
        
        theta = max_theta*(2*RanGen->Random()-1);
        calc_random_vector(axis); //Random unit vector to rotate around
        quat[0] = cos(theta/2); 
        for (j=0;j<3;j++) {
            quat[j+1] = axis[j]*sin(theta/2);
        }
        for (i=0;i<nAtoms;i++) {
            dist1[0] = beads[first+i].rn[0] - xCenter;
            dist1[1] = beads[first+i].rn[1] - yCenter;
            dist1[2] = beads[first+i].rn[2] - zCenter;

            quat_vec_rot(dist2,dist1,quat);

            update_chain[i].rn[0] = xCenter + dist2[0];
            update_chain[i].rn[1] = yCenter + dist2[1];
            update_chain[i].rn[2] = zCenter + dist2[2];

            PBC_shift(update_chain[i].r,update_chain[i].rn);

            quat_vec_rot(update_chain[i].u,save[i].u,quat);

            update_chain[i].grid = mapToGrid(update_chain[i].r);
            if ((update_chain[i].rn[2] < 0.0 || update_chain[i].rn[2] > box.z) && box.hasWall) {
                noWallBreak = false;
            }
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == update_chain[i].grid) flag = false;
            }
            if (flag) grid_ids.push_back(update_chain[i].grid);

        }
        
        if(noWallBreak) {
            //Save any grids that will be affected by the move
            pre_energy = 0.0;
            for (i=0;i<grid_ids.size();i++) {
                save_grid[i] = grid[grid_ids[i]];
                pre_energy += calc_grid_energy(grid_ids[i]);
            }

            for (i=0;i<nAtoms;i++) {
                removeBeadFromGrid(first+i);
                beads[first+i] = update_chain[i];
                addBeadToGrid(first+i);
            }

            post_energy = 0.0;
            for (i=0;i<grid_ids.size();i++) {
                post_energy += calc_grid_energy(grid_ids[i]);
            }

            dE = post_energy - pre_energy;
        }
        if (RanGen->Random() < exp(-dE) && noWallBreak) { //Accept changes
            box.E_tot += dE; 
            box.numAccepts++;
        } else { 
            //Restore Beads
            for (i=0;i<nAtoms;i++) {
                beads[first+i] = save[i];
            }
            //Restore Grids
            if (noWallBreak) {
                for (i=0;i<grid_ids.size();i++) {
                    grid[grid_ids[i]] = save_grid[i];
                }
            }
            box.numRejects++;
        }
    
        delete [] save;
        delete [] update_chain;
        delete [] save_grid;
        grid_ids.clear();
    }


}


void Sim::MC_pivot() {

    int moly,i,j,k,dir,core,disk_length,ref,first,last,tot_move,nAtoms;
    double x,y,z,xCenter,yCenter,zCenter,theta;   
    double pre_energy, post_energy, dE;
    LAMMPS_NS::vector dist1; // Contains distance vector for each disk from center of mass for rotation
    LAMMPS_NS::vector dist2; // Contains new distance vector for each disk from center of mass for rotation
    LAMMPS_NS::vector axis; // Contains new distance vector for each disk from center of mass for rotation
    quaternion quat;
    double max_theta = box.pivotMax;
    std::vector<int> grid_ids; //ID's of grids affected by the monte carlo move
    double a,b;
    bool flag,noWallBreak;


    for (int move=0;move<box.numMolecules;move++) {
        
        noWallBreak = true;

        do {
            moly = double(box.numMolecules)*RanGen->Random();
        } while(moly >= box.numMolecules);
        
        first = moleculeList[moly][0];
        last = moleculeList[moly][1];
        nAtoms = last-first+1;
        ref = first;

        do {
            core = double(nAtoms)*RanGen->Random();
        } while(core >= nAtoms);
        
        core += ref; //id of atom around which rotation will occur

        if (RanGen->Random() > 0.5) {
            dir = 1;
            first = core;
        } else {
            dir = -1;
            last = core;
        }

        nAtoms = last-first+1;

        Bead *save = new Bead[nAtoms];
        Bead *update_chain = new Bead[nAtoms];
        Grid *save_grid = new Grid[2*nAtoms]; //Maximum number of grids affected are nAtoms for initial grids, and nAtoms for final grids

        for (i=0;i<nAtoms;i++) {
            save[i] = beads[first+i];
            update_chain[i] = beads[first+i];
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == save[i].grid) flag = false;
            }
            if (flag) grid_ids.push_back(save[i].grid);

            if (beads[first+i].isAnchor && first+i != core) noWallBreak = false;
        }
       
        theta = max_theta*(2*RanGen->Random()-1);
        calc_random_vector(axis); //Random unit vector to rotate around
        quat[0] = cos(theta/2); 
        for (j=0;j<3;j++) {
            quat[j+1] = axis[j]*sin(theta/2);
        }
        
        for (i=0;i<nAtoms;i++) {
            dist1[0] = beads[first+i].rn[0] - beads[core].rn[0];
            dist1[1] = beads[first+i].rn[1] - beads[core].rn[1];
            dist1[2] = beads[first+i].rn[2] - beads[core].rn[2];

            quat_vec_rot(dist2,dist1,quat);

            update_chain[i].rn[0] = beads[core].rn[0] + dist2[0];
            update_chain[i].rn[1] = beads[core].rn[1] + dist2[1];
            update_chain[i].rn[2] = beads[core].rn[2] + dist2[2];

            PBC_shift(update_chain[i].r,update_chain[i].rn);

            quat_vec_rot(update_chain[i].u,save[i].u,quat);

            update_chain[i].grid = mapToGrid(update_chain[i].r);
            if ((update_chain[i].rn[2] < 0.0 || update_chain[i].rn[2] > box.z) && box.hasWall) {
                noWallBreak = false;
            }
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == update_chain[i].grid) flag = false;
            }
            if (flag) grid_ids.push_back(update_chain[i].grid);

        }
        

        
        if(noWallBreak) {
            //Save any grids that will be affected by the move
            pre_energy = calc_bead_energy(core); //Only bead that has angles change is the one acting as the center of rotation
            for (i=0;i<grid_ids.size();i++) {
                save_grid[i] = grid[grid_ids[i]];
                pre_energy += calc_grid_energy(grid_ids[i]);
            }

            for (i=0;i<nAtoms;i++) {
                removeBeadFromGrid(first+i);
                beads[first+i] = update_chain[i];
            }

            update_single_u_vector(core);

            for (i=0;i<nAtoms;i++) {
                addBeadToGrid(first+i);
            }

            post_energy = calc_bead_energy(core);
            for (i=0;i<grid_ids.size();i++) {
                post_energy += calc_grid_energy(grid_ids[i]);
            }

            dE = post_energy - pre_energy;
        }
        if (RanGen->Random() < exp(-dE) && noWallBreak) { //Accept changes
            box.E_tot += dE; 
            box.numAccepts++;
        } else { 
            //Restore Beads
            for (i=0;i<nAtoms;i++) {
                beads[first+i] = save[i];
            }
            //Restore Grids
            if(noWallBreak) {
                for (i=0;i<grid_ids.size();i++) {
                    grid[grid_ids[i]] = save_grid[i];
                }
            }
            box.numRejects++;
        }
    
        delete [] save;
        delete [] update_chain;
        delete [] save_grid;
        grid_ids.clear();
    }


}

void Sim::MC_reptate() { //Performs a reptation move on the polymer. Does not change any bond lengths, so it can be used for rigid polymers
   /* 
    int moly,i,j,k,dir,core,disk_length,ref,first,last,tot_move,nAtoms;
    double x,y,z,xCenter,yCenter,zCenter;   
    double pre_energy, post_energy, dE;
    LAMMPS_NS::vector dist1; // Contains distance vector for each disk from center of mass for rotation
    LAMMPS_NS::vector axis; // Contains new distance vector for each disk from center of mass for rotation
    double max_dist = box.translateMax;
    std::vector<int> grid_ids; //ID's of grids affected by the monte carlo move
    double a,b,bond_length;
    bool flag,noWallBreak;


    for (int move=0;move<box.numMolecules;move++) {
        
        noWallBreak = true;

        do {
            moly = double(box.numMolecules)*RanGen->Random();
        } while(moly >= box.numMolecules);
        
        first = moleculeList[moly][0];
        last = moleculeList[moly][1];
        nAtoms = last-first+1;

        Bead *save = new Bead[nAtoms];
        Bead *update_chain = new Bead[nAtoms];
        Grid *save_grid = new Grid[nAtoms+1]; //There may be one additional grid point included due to the reptation. All others affected will aleady be there

        for (i=0;i<nAtoms;i++) {
            save[i] = beads[first+i];
            update_chain[i] = beads[first+i];
            flag = true;
            for (j=0;j<grid_ids.size();j++) {
                if (grid_ids[j] == save[i].grid) flag = false;
            }
            if (flag) grid_ids.push_back(save[i].grid);

            if (beads[first+i].isAnchor) noWallBreak = false;
        }
      
        //Need to calculate the correct length of the bond that is being moved from one end to another
        

        if (RanGen->Random() < 0.5) { //New bead is attached onto last bead. First bead in chain is removed
            dir = 1;
            bond_length = 0.0;
            bond_length += (beads[first].xn - beads[first+1].xn) * (beads[first].xn - beads[first+1].xn);
            bond_length += (beads[first].yn - beads[first+1].yn) * (beads[first].yn - beads[first+1].yn);
            bond_length += (beads[first].zn - beads[first+1].zn) * (beads[first].zn - beads[first+1].zn);
        } else {                      //New bead is attached onto first bead. Last bead in chain is removed
            dir = -1;
            bond_length = 0.0;
            bond_length += (beads[last].xn - beads[last-1].xn) * (beads[last].xn - beads[last-1].xn);
            bond_length += (beads[last].yn - beads[last-1].yn) * (beads[last].yn - beads[last-1].yn);
            bond_length += (beads[last].zn - beads[last-1].zn) * (beads[last].zn - beads[last-1].zn);
        }

        //Generate a random vector that is bond_length long
        

        calc_random_vector(axis);

        for (i=0;i<3;i++) {
            axis[i] = bond_length*axis[i];
        }


        x = max_dist*(2*RanGen->Random()-1.0); //Calculation for new center of mass
        y = max_dist*(2*RanGen->Random()-1.0); //Calculation for new center of mass
        z = max_dist*(2*RanGen->Random()-1.0); //Calculation for new center of mass
       

        
        for (j=0;j<nAtoms;j++) {
            update_chain[j].rn[0] += x;
            update_chain[j].rn[1] += y;
            update_chain[j].rn[2] += z;
            PBC_shift(update_chain[j].r,update_chain[j].rn);
            update_chain[j].grid = mapToGrid(update_chain[j].r);
            if ((update_chain[j].rn[2] < 0.0 || update_chain[j].rn[2] > box.z) && box.hasWall) {
                noWallBreak = false;
            }
            flag = true;
            for (i=0;i<grid_ids.size();i++) {
                if (grid_ids[i] == update_chain[j].grid) flag = false;
            }
            if (flag) grid_ids.push_back(update_chain[j].grid);
    

        }

        if (noWallBreak) {
            //Save any grids that will be affected by the move
            pre_energy = 0.0;
            for (i=0;i<grid_ids.size();i++) {
                save_grid[i] = grid[grid_ids[i]];
                pre_energy += calc_grid_energy(grid_ids[i]);
            }

            for (i=0;i<nAtoms;i++) {
                removeBeadFromGrid(first+i);
                beads[first+i] = update_chain[i];
                addBeadToGrid(first+i);
            }

            post_energy = 0.0;
            for (i=0;i<grid_ids.size();i++) {
                post_energy += calc_grid_energy(grid_ids[i]);
            }

            dE = post_energy - pre_energy;
        }
        if (RanGen->Random() < exp(-dE) && noWallBreak) { //Accept changes
            box.E_tot += dE; 
            box.numAccepts++;
        } else { 
            //Restore Beads
            for (i=0;i<nAtoms;i++) {
                beads[first+i] = save[i];
            }
            //Restore Grids
            for (i=0;i<grid_ids.size();i++) {
                grid[grid_ids[i]] = save_grid[i];
            }
            box.numRejects++;
        }
    
        delete [] save;
        delete [] update_chain;
        delete [] save_grid;

        grid_ids.clear();
    }
    
   
*/
}
/**************************************************************************************************************/


/**********************************DATA OUTPUT AND VISUALIZATION***********************************************/
void Sim::shiftCOM() { //Shifts the center of mass of all the nonperiodic coordinates back into the simulation box 
    int i,j,m,start,end,nAtoms,flag;
    double xCenter,yCenter,zCenter,dx,dy,dz;
    for (i=0;i<box.numMolecules;i++) {
        flag = 0;
        xCenter = 0.0;
        yCenter = 0.0;
        zCenter = 0.0;
        start = moleculeList[i][0];
        end = moleculeList[i][1];
        nAtoms = end-start+1;
        for (j=start;j<=end;j++) {
            if (beads[j].isAnchor) {
                flag = 1;
            }
            xCenter += beads[j].rn[0];
            yCenter += beads[j].rn[1];
            zCenter += beads[j].rn[2];
        }
        if (flag == 1) {
            continue;
        }
        xCenter /= double(nAtoms);
        yCenter /= double(nAtoms);
        zCenter /= double(nAtoms);
        dx = box.x*floor(xCenter/box.x);
        dy = box.y*floor(yCenter/box.y);
        dz = box.z*floor(zCenter/box.z);
        for (j=start;j<=end;j++) {
            beads[j].rn[0] -= dx;
            beads[j].rn[1] -= dy;
            beads[j].rn[2] -= dz;
        }
    }
}

void Sim::printXYZ() {
    //Print all the molecules then print out rings to visualize disk-like "atoms"
    
    int i,j;
    FILE *dump;
    dump = fopen("config.xyz","a");

    fprintf(dump,"%d\nConfiguratoin\n",box.numBeads);
    for (i=0;i<box.numBeads;i++) {
        fprintf(dump,"0\t%f\t%f\t%f\n",beads[i].rn[0],beads[i].rn[1],beads[i].rn[2]);
    }
    fclose(dump);
}

void Sim::printPSF() { //Print out the PSF file. Only visualizing bonds here, not twist or angle potentials
      FILE *sout;
      int counter,type,pair,chain_switch,bead_type;
      char t[2]="O";
      sout=fopen("polymer.psf","w");
      fprintf(sout,"*\n*\n*\n*\n*\n\n");
      fprintf(sout,"%7d !NATOMS\n",box.numBeads);
      for(counter=1;counter<=box.numBeads;counter++){
        if(counter<=box.numBeads){

        if (beads[counter-1].type == 0) t[0] = 'O'; 
        if (beads[counter-1].type == 1) t[0] = 'N'; 
        if (beads[counter-1].type == 2) t[0] = 'H'; 
        if (beads[counter-1].type == 3) t[0] = 'P'; 

          bead_type=beads[counter-1].type;

          fprintf(sout, "%8d ", counter);
          fprintf(sout, "POLY ");
          fprintf(sout, "%-4d ",1);
          fprintf(sout, "%s  ", "POL");
          fprintf(sout, "%-5s ",t);
          fprintf(sout, "%3d  ", bead_type);
          fprintf(sout, "%13.6e   ",0.0);
          fprintf(sout, "%7.3lf           0\n", 1.0);
        }

        else{
          t[0]='S'; bead_type=2;
          fprintf(sout, "%8d ", counter);
          fprintf(sout, "SOLV ");
          fprintf(sout, "%-4d ",1);
          fprintf(sout, "%s  ", "SOL");
          fprintf(sout, "%-5s ",t);
          fprintf(sout, "%3d  ", bead_type);
          fprintf(sout, "%13.6e   ",0.0);
          fprintf(sout, "%7.3lf           0\n", 1.0);

        }        
        //fprintf(sout,"%8d\tU\t%4d\tPOL\t%1s\t%1s\t0.0000000\t1.0000\t0\n  ",counter,counter-1,t,t);
      }


      fprintf(sout,"\n%8d !NBOND: bond\n",box.numBonds);




      pair=1;
      counter=1;
      int i;
      for(i=0;i<box.numBonds;i++){

        if(pair==5)
          {pair=1;fprintf(sout,"\n");}
        //printf("**counter:\t%d\n",counter);
        fprintf(sout,"%8d%8d",bonds[i].id1+1,bonds[i].id2+1);
        pair++;
      }

      fprintf(sout,"\n%8d !NTHETA: angles\n",0);
      fprintf(sout,"\n%8d !NPHI: dihedrals\n",0);
      fprintf(sout,"\n%8d !NIMPR\n",0);
      fprintf(sout,"\n%8d !HDON\n",0);
      fprintf(sout,"\n%8d !HACC\n",0);
      fprintf(sout,"\n%8d !NNB\n",0);


      fclose(sout);

}


void Sim::empty_containers() {
    bondList.clear();
    angleList.clear();
    moleculeList.clear();
}

/*(
void Sim::printPOV() {
    FILE *pov = fopen("disks.xyz","w");
    fprintf(pov, "ITEM: TURN\n%d\nITEM: NUMBER OF ATOMS\n%d\nITEM: BOX BOUNDS\n-10 10\n-10 10\n-10 10\nITEM: ATOMS\n",box.cycle,box.numTotal);

    double scalex = 10.0/(box.x/2);
    double scaley = 10.0/(box.y/2);
    double scalez = 10.0/(box.z/2);

    for (int i=0;i<box.numTotal;i++) {
        fprintf(pov,"%d 1 %f %f %f %f %f %f\n", i, (disk[i].r[0]-box.x/2)*scalex, (disk[i].r[1]-box.y/2)*scaley, (disk[i].r[2]-box.z/2)*scalez, disk[i].f[0], disk[i].f[1], disk[i].f[2]); 
    }
    fclose(pov);
}
*/

void Sim::writeEnergy() {
    
    int i,j,k;
    int t1,t2;
    double bond, angle, compression, chi, nematic;
    double phi_tot, chi_single;
    bond = 0.0; compression = 0.0; nematic = 0.0; chi = 0.0;
    for (i=0;i<box.numBonds;i++) {
        bond += calc_bond_energy(i); 
    }
    for (i=0;i<box.numAngles;i++) {
        angle += calc_angle_energy(i);
    }
    for (i=0;i<box.numGrid;i++) {
        phi_tot = 0.0;
        chi_single = 0.0;
        for (t1=0;t1<box.numTypes;t1++) {
            phi_tot += grid[i].phi[t1];
        }

        compression += 0.5*box.kappa*phi_tot*phi_tot*grid[i].vol*box.dens*box.Delta;

        for (t1=0;t1<box.numTypes;t1++) {
            for (t2=t1+1;t2<box.numTypes;t2++) {
                chi_single += chis[t1][t2]*grid[i].phi[t1]*grid[i].phi[t2];
            }
        }

        chi += chi_single*grid[i].vol*box.dens*box.Delta;
        if (grid[i].vol ==  0.0) {
            nematic += 0.0;
        } else {
            for (j=0;j<grid[i].bead_list.size();j++) {
                for (k=j+1;k<grid[i].bead_list.size();k++) {
                    nematic += calc_nematic_pair_energy(grid[i].bead_list[j],grid[i].bead_list[k])*box.Delta/(box.dens*grid[i].vol);
                }
            }   
        }

    }
    
    FILE *en = fopen("Energy.txt","a");
    fprintf(en,"%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",box.cycle,bond,angle,compression,chi,nematic,bond+angle+compression+chi+nematic,box.E_tot);
    fclose(en);
}

void Sim::dumpData() {
    FILE *out = fopen("data.txt","a");
    int i;
    fprintf(out,"%d\n",box.cycle);
    for (i=0;i<box.numBeads;i++) {
        fprintf(out,"%d\t", beads[i].id);
        fprintf(out,"%lf\t%lf\t%lf\t", beads[i].r[0], beads[i].r[1], beads[i].r[2]);
        fprintf(out,"%lf\t%lf\t%lf\t", beads[i].rn[0], beads[i].rn[1], beads[i].rn[2]);
        fprintf(out,"%lf\t%lf\t%lf\t", beads[i].u[0], beads[i].u[1], beads[i].u[2]);
        fprintf(out,"%d\t", beads[i].type);
        fprintf(out,"%d\t", beads[i].isStart);
        fprintf(out,"%d\t", beads[i].isEnd);
        fprintf(out,"%d\n", beads[i].isAnchor);
    }
    fclose(out);
}

/**************************************************************************************************************/


/**************************************************************************************************************/
