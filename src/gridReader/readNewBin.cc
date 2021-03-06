// System includes.
#include <rpc/rpc.h>
#include <rpc/xdr.h>

// Standard library includes.
#include <map>
#include <vector>
using std::map ;
using std::vector ;

// Loci includes.
#include <Loci.h>
#include <Tools/ftrn_reader.h>
#include <Tools/stream.h>
#include <Tools/tools.h>
using Loci::fortran_binary_file ;
using Loci::all_collect_vectors ;
using Loci::all_collect_entitySet ;
using Loci::MPI_rank ;
using Loci::fill_clone ;
using Loci::GLOBAL_MIN ;

// StreamUns includes.
#include "name_var.h"
#include "readGrid.h"
#include "sciTypes.h"

// We want scalable output.
#define SCALABLE_OUTPUT

// Forward declaration of Metis partitioner.
extern "C" {
  typedef int idxtype ;
  void ParMETIS_PartKway(idxtype*,idxtype*,idxtype*,idxtype*,idxtype*,int*,
    int*,int*,int*,int*,idxtype*,MPI_Comm*) ;
}

namespace streamUns {

  void ReadNewBin(fact_db &facts, const char *filename, bool binary_file) {
    int recv_data[3] ;
    int ndm, nzones, npatch ;
    int npnts, nfaces, ncells, maxppf, maxfpc ;
    int *send_data = new int[Loci::MPI_processes*3] ;
    FILE* FP = 0 ;
    XDR xdr_handle ;
    if(Loci::MPI_rank == 0 ) {
      cout << " reading file = " << filename << endl ;
      cout << "NEW BINARY Grid File input" << endl ;
      if(!binary_file) {
	cerr << " The input file is not binary " << endl ;
	exit(-1) ;
      }
      char buf[512] ;
      sprintf(buf,"%s.xdr",filename) ;
      FP = fopen(buf, "r") ;
      if(FP == NULL) {
	cerr << "Can't open file " << buf << "'" << endl ;
	exit(-1) ;
      }
      
      xdrstdio_create(&xdr_handle, FP, XDR_DECODE) ;
      xdr_int(&xdr_handle, &ndm) ;
      xdr_int(&xdr_handle, &nzones) ;
      xdr_int(&xdr_handle, &npatch) ;
      
      int tmp_size = 0 ;
      
      xdr_int(&xdr_handle, &npnts) ;
      xdr_int(&xdr_handle, &nfaces) ;
      xdr_int(&xdr_handle, &ncells) ;
      xdr_int(&xdr_handle, &maxppf) ;
      xdr_int(&xdr_handle, &maxfpc) ;
      for(int i = 0; i < Loci::MPI_processes; ++i) {
	send_data[tmp_size++] = npnts ;
	send_data[tmp_size++] = nfaces ;
	send_data[tmp_size++] = ncells ;
      }	
      if(Loci::MPI_processes > 1)
	MPI_Scatter(send_data,3,MPI_INT,recv_data,3,MPI_INT,0,MPI_COMM_WORLD) ;
    }
    else {
      MPI_Scatter(send_data,3,MPI_INT,recv_data,3,MPI_INT,0,MPI_COMM_WORLD) ;
      npnts = recv_data[0] ;
      nfaces = recv_data[1] ;
      ncells = recv_data[2] ;
    }
    delete [] send_data ;
    int max_alloc = 0 ;
    max_alloc = facts.get_max_alloc() ;
    int node_ivl = npnts / Loci::MPI_processes;
    int face_ivl = nfaces / Loci::MPI_processes;
    int cell_ivl = ncells / Loci::MPI_processes;
    
    std::vector<entitySet> local_nodes(Loci::MPI_processes) ;
    std::vector<entitySet> local_faces(Loci::MPI_processes) ;
    std::vector<entitySet> local_cells(Loci::MPI_processes) ;
    for(int i = 0; i < Loci::MPI_processes; ++i) {
      if(i == Loci::MPI_processes-1) {
	local_nodes[i] = interval(max_alloc+i*node_ivl,max_alloc+npnts-1) ;
	local_faces[i] = interval(max_alloc+npnts+i*face_ivl,max_alloc+npnts+
          nfaces-1) ;
	local_cells[i] = interval(max_alloc+npnts+nfaces+i*cell_ivl,max_alloc+
          npnts+nfaces+ncells-1) ;
      }
      else {
	local_nodes[i] = interval(max_alloc+i*node_ivl,max_alloc+i*node_ivl+
          node_ivl-1) ;
	local_faces[i] = interval(max_alloc+npnts+i*face_ivl,max_alloc+npnts+
          i*face_ivl + face_ivl - 1) ;
	local_cells[i] = interval(max_alloc+npnts+nfaces+i*cell_ivl,max_alloc+
          npnts+nfaces+i*cell_ivl+cell_ivl-1) ;
      }
    }
    dstore<vect3d> t_pos ;
    if(Loci::MPI_rank == 0) {
      double *tmp_pos ;
      tmp_pos = new double[local_nodes[Loci::MPI_processes-1].size() * 3] ;
      for(int i = 0; i < 3*local_nodes[Loci::MPI_rank].size(); ++i)
	xdr_double(&xdr_handle, &tmp_pos[i]) ;
      int tmp = 0 ;
      for(entitySet::const_iterator ei=local_nodes[Loci::MPI_rank].begin();
      ei!=local_nodes[Loci::MPI_rank].end();++ei) {
	vect3d t(tmp_pos[tmp], tmp_pos[tmp+1], tmp_pos[tmp+2]) ;
	tmp += 3 ;
	t_pos[*ei] = t ; //tmp_pos[tmp] ;
      }
      int init_pack_size = t_pos.Rep()->pack_size(local_nodes[Loci::
        MPI_processes-1]) ;
      unsigned char* tmp_buf = new unsigned char[init_pack_size] ;
      for(int i = 1; i < Loci::MPI_processes; ++i) { 
	for(int j = 0; j < 3*local_nodes[i].size(); ++j)
	  xdr_double(&xdr_handle, &tmp_pos[j]) ;
	int pack_size = t_pos.Rep()->pack_size(local_nodes[i]) ;
	int loc_pack = 0 ;
	MPI_Pack(tmp_pos,local_nodes[i].size()*sizeof(vect3d),MPI_BYTE,tmp_buf,
          init_pack_size, &loc_pack, MPI_COMM_WORLD) ;
	MPI_Send(tmp_buf, pack_size, MPI_PACKED, i, 9, MPI_COMM_WORLD) ; 
      }
      delete [] tmp_pos ;
      delete [] tmp_buf ;
    }
    else {
      MPI_Status status ;
      int recv_count = t_pos.Rep()->pack_size(local_nodes[Loci::MPI_rank]) ;
      unsigned char *tmp_buf = new unsigned char[recv_count] ;
      MPI_Recv(tmp_buf, recv_count, MPI_PACKED, 0, 9, MPI_COMM_WORLD, &status) ;  
      int loc_unpack = 0 ;
      Loci::sequence tmp_seq = Loci::sequence(local_nodes[Loci::MPI_rank]) ;
      t_pos.Rep()->unpack(tmp_buf, loc_unpack, recv_count, tmp_seq) ; 
      delete [] tmp_buf ;
    }
    dMap tmp_cl, tmp_cr ;
    store<int> local_count ;
    dmultiMap tmp_face2node ;
    local_count.allocate(local_faces[Loci::MPI_rank]) ;
    std::vector<int> offset ;
    std::vector<int> start_off(Loci::MPI_processes+1) ;
    start_off[0] = 0 ;
    if(Loci::MPI_rank == 0) {
      int *off_cl_cr ;
      off_cl_cr = new int[3*local_faces[Loci::MPI_processes-1].size() + 1] ;
      //fread(&off_cl_cr[0], sizeof(int), (local_faces[0].size() * 3) + 1, FP) ; 
      for(int i = 0; i < (local_faces[0].size() * 3) + 1; ++i)
	xdr_int(&xdr_handle, &off_cl_cr[i]) ;
      int tmp = 0 ;
      for(entitySet::const_iterator ei=local_faces[0].begin();ei!=
      local_faces[0].end();++ei) {
	offset.push_back(off_cl_cr[tmp++]) ;
	tmp_cl[*ei] = off_cl_cr[tmp++] ;
	tmp_cr[*ei] = off_cl_cr[tmp++] ;
	if(tmp_cl[*ei] < 0) 
	  if(tmp_cr[*ei] < 0) {
	    cerr << " boundary condition on both sides of a face?" << endl ;
	    exit(1) ;
	  } else {
	    int tmp_swap = tmp_cr[*ei] ;
	    tmp_cr[*ei] = tmp_cl[*ei] ;
	    tmp_cl[*ei] = tmp_swap ;
	  }
	tmp_cl[*ei] += max_alloc + npnts + nfaces - 1 ;
	if(tmp_cr[*ei] > 0) 
	  tmp_cr[*ei] += max_alloc + npnts + nfaces - 1 ;
      }
      offset.push_back(off_cl_cr[tmp]) ;
      entitySet::const_iterator ii = local_faces[Loci::MPI_rank].begin() ;
      for(size_t i = 1; i < offset.size(); ++i) {
	local_count[*ii] = offset[i] - offset[i-1] ;
	++ii ;
      }
      
      int init_off = off_cl_cr[tmp] ;
      for(int i = 1; i < Loci::MPI_processes; ++i) { 
	off_cl_cr[0] = init_off ;
	start_off[i] = init_off ;
	for(int j = 1; j < (local_faces[i].size() * 3) +1; ++j)
	  xdr_int(&xdr_handle, &off_cl_cr[j]) ;
	int send_size = local_faces[i].size() * 3 + 1 ;
	MPI_Send(off_cl_cr, send_size, MPI_INT, i, 10, MPI_COMM_WORLD) ; 
	init_off = off_cl_cr[local_faces[i].size()*3] ;
	if(i==Loci::MPI_processes-1)
	  start_off[Loci::MPI_processes] = init_off ; 
      }
      delete [] off_cl_cr ;
    }
    else {
      MPI_Status status ;
      int recv_count = local_faces[Loci::MPI_rank].size() * 3 + 1 ;
      int *off_cl_cr = new int[3*local_faces[Loci::MPI_rank].size() + 1] ;
      MPI_Recv(off_cl_cr, recv_count, MPI_INT, 0, 10, MPI_COMM_WORLD, &status) ;  
      int tmp = 0 ;
      for(entitySet::const_iterator ei=local_faces[Loci::MPI_rank].begin();
      ei!=local_faces[Loci::MPI_rank].end();++ei) {
	offset.push_back(off_cl_cr[tmp++]) ;
	tmp_cl[*ei] = off_cl_cr[tmp++] ;
	tmp_cr[*ei] = off_cl_cr[tmp++] ;
	if(tmp_cl[*ei] < 0) 
	  if(tmp_cr[*ei] < 0) {
	    cerr << "2 boundary condition on both sides of a face?" << endl ;
	    exit(1) ;
	  } else {
	    int tmp = tmp_cr[*ei] ;
	    tmp_cr[*ei] = tmp_cl[*ei] ;
	    tmp_cl[*ei] = tmp ;
	  }
	tmp_cl[*ei] += max_alloc + npnts + nfaces - 1 ;
	if(tmp_cr[*ei] > 0) 
	  tmp_cr[*ei] += max_alloc + npnts + nfaces - 1 ;
      }
      offset.push_back(off_cl_cr[tmp]) ;
      entitySet::const_iterator ii = local_faces[Loci::MPI_rank].begin() ;
      for(size_t i = 1; i < offset.size(); ++i) {
	local_count[*ii] = offset[i] - offset[i-1] ;
	++ii ;
      }
      delete [] off_cl_cr ;
    }
    tmp_face2node.allocate(local_count) ;
    if(Loci::MPI_processes > 1) {
      if(Loci::MPI_rank == 0) {
	int* tmp_f2n=new int[local_faces[Loci::MPI_processes-1].size()*maxppf] ;
	for(int i=0;i<(start_off[Loci::MPI_rank+1]-start_off[Loci::MPI_rank]);
          ++i) xdr_int(&xdr_handle, &tmp_f2n[i]) ;
	int tmp = 0 ;
	for(entitySet::const_iterator ei=local_faces[0].begin();ei!=
        local_faces[0].end(); ++ei) 
	  for(int i = 0; i < local_count[*ei]; ++i)
	    tmp_face2node[*ei][i] = tmp_f2n[tmp++] ;
	for(int i = 1; i < Loci::MPI_processes; ++i) { 
	  int send_size = start_off[i+1] - start_off[i] ; 
	  for(int j = 0; j < send_size; ++j)
	    xdr_int(&xdr_handle, &tmp_f2n[j]) ;
	  MPI_Send(tmp_f2n, send_size, MPI_INT, i, 11, MPI_COMM_WORLD) ; 
	}
	delete [] tmp_f2n ;
      }
      else {
	MPI_Status status ;
	int recv_count = offset[offset.size()-1] - offset[0] ;
	int *tmp_f2n = new int[recv_count] ;
	MPI_Recv(tmp_f2n, recv_count, MPI_INT, 0, 11, MPI_COMM_WORLD, &status) ;  
	int tmp = 0 ;
	for(entitySet::const_iterator ei=local_faces[Loci::MPI_rank].begin();
        ei!=local_faces[Loci::MPI_rank].end();++ei) 
	  for(int i = 0; i < local_count[*ei]; ++i)
	    tmp_face2node[*ei][i] = tmp_f2n[tmp++] ;
	delete [] tmp_f2n ;
      }
      entitySet nodes = interval(max_alloc, max_alloc + npnts-1) ;
      entitySet faces=interval(max_alloc+npnts,max_alloc+npnts+nfaces-1) ;
      entitySet cells=interval(max_alloc+npnts+nfaces,max_alloc+npnts+nfaces+
        ncells-1) ;
      fact_db local_facts ;
      std::pair<entitySet, entitySet> node_pair, face_pair, cell_pair ;
      local_facts.set_maximum_allocated(max_alloc) ;
      node_pair=local_facts.get_distributed_alloc(local_nodes[Loci::MPI_rank].
        size()) ;
      face_pair=local_facts.get_distributed_alloc(local_faces[Loci::MPI_rank].
        size()) ;
      cell_pair=local_facts.get_distributed_alloc(local_cells[Loci::MPI_rank].
        size()) ;
      entitySet cri = Loci::MapRepP(tmp_cr.Rep())->image(faces) ;
      entitySet tmp_orig_boundaries =  cri & interval(Loci::UNIVERSE_MIN,-1) ;
      entitySet orig_boundaries=Loci::all_collect_entitySet
        (tmp_orig_boundaries) ;
      dmultiMap cl_inverse, cr_inverse ;
      entitySet bound_faces = tmp_cr.preimage(tmp_orig_boundaries).first ;
      entitySet interior_faces = local_faces[Loci::MPI_rank] - bound_faces ;
      entitySet global_interior_faces = all_collect_entitySet(interior_faces) ;
      std::vector<entitySet> &init_ptn = local_facts.get_init_ptn() ;
      Loci::distributed_inverseMap(cl_inverse,tmp_cl,cells,
        global_interior_faces, init_ptn) ;
      Loci::distributed_inverseMap(cr_inverse,tmp_cr,cells,
        global_interior_faces, init_ptn) ;
      dmultiMap tmp_cl_inverse ;
      Loci::distributed_inverseMap(tmp_cl_inverse,tmp_cl,cells,faces,init_ptn) ;
      store<name_var> boundary_names ;
      boundary_names.allocate(orig_boundaries) ;
      if(Loci::MPI_rank == 0) 
	cout << "boundaries identified as:" ;
      FORALL(orig_boundaries, bc) {
	char buf[512] ;
	sprintf(buf,"BC_%d",-bc) ;
	boundary_names[bc].name = buf ;
	if(Loci::MPI_rank == 0)
	  cout << " " << boundary_names[bc].name ;
      } ENDFORALL ;
      if(Loci::MPI_rank == 0)
	cout << endl ;
      std::vector<entitySet> vset = Loci::all_collect_vectors
        (tmp_orig_boundaries) ;
      for(int i = 0; i < Loci::MPI_processes; ++i) 
	init_ptn[i] += vset[i] ; 
      
      local_facts.create_fact("tmp_cl", tmp_cl) ;
      local_facts.create_fact("tmp_cr", tmp_cr) ;
      local_facts.create_fact("tmp_face2node", tmp_face2node) ;
      fact_db::distribute_infoP df ;
      if(!facts.is_distributed_start()) {
	df = new fact_db::distribute_info  ;
	df->chop_ptn = init_ptn ;
      }
      else {
	df = facts.get_distribute_info() ;
	for(int i = 0; i < Loci::MPI_processes; ++i)
	  (df->chop_ptn)[i] += init_ptn[i] ;
      }
      
      entitySet cl_out, cr_out ;
      entitySet cl_inv_ran=Loci::MapRepP(cl_inverse.Rep())->image
        (local_cells[Loci::MPI_rank]) ;
      cr_out  = cl_inv_ran - local_faces[Loci::MPI_rank] ; 
      Loci::storeRepP crsp=Loci::MapRepP(tmp_cr.Rep())->expand(cr_out,
        init_ptn) ;
      entitySet cr_inv_ran=Loci::MapRepP(cr_inverse.Rep())->image
        (local_cells[Loci::MPI_rank]) ;
      cl_out  = cr_inv_ran - local_faces[Loci::MPI_rank] ; 
      Loci::storeRepP clsp=Loci::MapRepP(tmp_cl.Rep())->expand(cl_out,
        init_ptn) ;
      Loci::MapRepP(cl_inverse.Rep())->compose(tmp_cr,local_cells[Loci::
        MPI_rank]) ;
      Loci::MapRepP(cr_inverse.Rep())->compose(tmp_cl,local_cells[Loci::
        MPI_rank]) ;
      dmultiMap dynamic_map ;
      FORALL(local_cells[Loci::MPI_rank], lci) {
	std::vector<int> tmp_int ;
	for(size_t i = 0; i < cl_inverse[lci].size(); ++i)
	  tmp_int.push_back(cl_inverse[lci][i]) ;
	for(size_t i = 0; i < cr_inverse[lci].size(); ++i)
	  tmp_int.push_back(cr_inverse[lci][i]) ;
	dynamic_map[lci] = tmp_int ;
      } ENDFORALL ;
      int count = 0 ;
      int size_map = local_cells[Loci::MPI_rank].size() ;
      entitySet dom_map = Loci::interval(0, size_map-1) ;
      store<int> size_adj ;
      size_adj.allocate(dom_map) ;
      count = 0 ;
      for(entitySet::const_iterator ei=local_cells[Loci::MPI_rank].begin();ei!=
      local_cells[Loci::MPI_rank].end();++ei) {
	size_adj[count] = dynamic_map[*ei].size() ;  
	++count ;
      }
      int *part = new int[size_map] ;
      int *xadj = new int[size_map+1] ;
      int options, numflag, edgecut, wgtflag ;
      int *vdist = new int[Loci::MPI_processes + 1] ;
      int cmin = cells.Min() ;
      cmin = GLOBAL_MIN(cmin) ;
      options = 0 ;
      numflag = 0 ;
      wgtflag = 0 ;
      edgecut = 0 ;
      xadj[0] = 0 ;
      for(int i = 0; i < size_map; ++i) 
	xadj[i+1] = xadj[i] + size_adj[i] ;
      
      int *adjncy = new int[xadj[size_map]] ;
      count = 0 ;
      for(entitySet::const_iterator ei=local_cells[Loci::MPI_rank].begin();ei!=
      local_cells[Loci::MPI_rank].end();++ei) 
	for(size_t i = 0; i != dynamic_map[*ei].size(); ++i) {
	  adjncy[count] = dynamic_map[*ei][i] - cmin ;
	  count ++ ;
	}
      vdist[0] = 0 ;
      for(int i = 1; i <= Loci::MPI_processes; ++i) 
	vdist[i] = vdist[i-1] + local_cells[i-1].size() ;
      
      MPI_Barrier(MPI_COMM_WORLD) ;
      MPI_Comm mc = MPI_COMM_WORLD ;
      int num_partitions = Loci::MPI_processes ;
      ParMETIS_PartKway(vdist,xadj,adjncy,NULL,NULL,&wgtflag,&numflag,
        &num_partitions,&options,&edgecut,part, &mc) ;
      if(Loci::MPI_rank == 0)
	cout << " Parmetis Edge cut   " <<  edgecut << endl ;
      delete [] xadj ;
      delete [] adjncy ;
      delete [] vdist ;
      
      vector<entitySet> ptn ;
      std::vector<entitySet> tmp_init_ptn = init_ptn ;
      for(int i = 0; i < Loci::MPI_processes; ++i)
	ptn.push_back(EMPTY) ;
      cmin = local_cells[Loci::MPI_rank].Min() ;
      for(int i=0;i<size_map;++i) {
	ptn[part[i]] += i + cmin ;
      }
      delete [] part ;
      for(int i = 0; i < Loci::MPI_processes; ++i) {
	ptn[i] = all_collect_entitySet(ptn[i]) ;
	tmp_init_ptn[i] -= local_cells[i] ;
	tmp_init_ptn[i] += ptn[i] ; 
//Loci::debugout << " Num_cells[ " << i << " ] = " << ptn[i].size() << endl ;
      }
      entitySet req_face ;
      cl_out = ptn[Loci::MPI_rank] - local_cells[Loci::MPI_rank] ;
      Loci::storeRepP inverse_sp=Loci::MapRepP(tmp_cl_inverse.Rep())->
        expand(cl_out,init_ptn) ;
      cl_inv_ran = Loci::MapRepP(inverse_sp)->image(ptn[Loci::MPI_rank]) ;
      cl_out = cl_inv_ran - tmp_cl.domain() ;
      clsp = Loci::MapRepP(tmp_cl.Rep())->expand(cl_out, init_ptn) ;
      FORALL(cl_inv_ran, clo) {
	if(ptn[Loci::MPI_rank].inSet(tmp_cl[clo])) 
	  req_face += clo ;
      } ENDFORALL ;
      std::vector<entitySet> v_req = all_collect_vectors(req_face) ;
      std::vector<entitySet> bound_vec = all_collect_vectors(bound_faces) ;
      for(int i = 0; i < Loci::MPI_processes; ++i) {
	tmp_init_ptn[i] -= faces ;
	tmp_init_ptn[i] += v_req[i] ;
//Loci::debugout << " Num_faces[ " << i << " ] = " << v_req[i].size() << endl ;
      }
      entitySet f2n_out = v_req[Loci::MPI_rank] - tmp_face2node.domain() ;
      Loci::storeRepP f2n_sp=Loci::MapRepP(tmp_face2node.Rep())->expand(f2n_out,
        init_ptn) ; 
      
      entitySet my_nodes=Loci::MapRepP(tmp_face2node.Rep())->image
        (v_req[Loci::MPI_rank]) ; 
      v_req = all_collect_vectors(my_nodes) ;
      for(int i = 0; i < Loci::MPI_processes; ++i) {
	for(int j = i+1 ; j < Loci::MPI_processes; ++j) {
	  entitySet  tmp = v_req[i] & v_req[j] ;
	  v_req[j] -= tmp ;
	} 
	tmp_init_ptn[i] -= nodes ;
	tmp_init_ptn[i] += v_req[i] ;
	//Loci::debugout << "Num_nodes = " << v_req[i].size() << endl ;
      }
      entitySet loc_nodes, loc_faces, loc_cells ;
      loc_nodes = nodes & tmp_init_ptn[Loci::MPI_rank] ;
      loc_faces = faces & tmp_init_ptn[Loci::MPI_rank] ;
      loc_cells = cells & tmp_init_ptn[Loci::MPI_rank] ;
      entitySet gn = Loci::all_collect_entitySet(loc_nodes) ;
      cr_out = loc_faces - tmp_cr.domain() ;
      crsp = Loci::MapRepP(tmp_cr.Rep())->expand(cr_out, init_ptn) ;
      entitySet pos_out = loc_nodes - t_pos.domain() ;
      Loci::storeRepP t_pos_sp = t_pos.Rep() ;
      entitySet total_dom = t_pos.domain() + loc_nodes ;
      fill_clone(t_pos_sp, pos_out, init_ptn) ;
      node_pair = facts.get_distributed_alloc(loc_nodes.size()) ;
      face_pair = facts.get_distributed_alloc(loc_faces.size()) ;
      cell_pair = facts.get_distributed_alloc(loc_cells.size()) ;
      
      nodes = node_pair.first ;
      faces = face_pair.first ;
      cells = cell_pair.first ;
      Loci::debugout << "nodes = " << nodes << endl ;
      Loci::debugout << "faces = " <<faces << endl ;
      Loci::debugout << "cells = " << cells << endl ;
      std::vector<entitySet> &new_init_ptn = facts.get_init_ptn() ;
      
      Map remap_nodes, remap_cells ;
      remap_nodes.allocate(loc_nodes) ;
      remap_cells.allocate(loc_cells) ;
      dMap remap ;
      if(facts.is_distributed_start())
	remap = df->remap ;
      entitySet::const_iterator ei = loc_nodes.begin() ;
      FORALL(nodes, li) {
	remap_nodes[*ei] = li ;
	remap[*ei] = li ;
	++ei ;
      } ENDFORALL ;
      
      ei = loc_cells.begin() ;
      FORALL(cells, li) {
	remap_cells[*ei] = li ;
	remap[*ei] = li ;
	++ei ;
      } ENDFORALL ;
      
      df->remap = remap ;
      facts.put_distribute_info(df) ;
      Map cl, cr ;
      multiMap face2node ;
      store<vect3d> pos ;
      pos.allocate(nodes) ;
      store<int> new_count ;
      new_count.allocate(faces) ;
      cl.allocate(faces) ;
      cr.allocate(faces) ;
      
      std::vector<entitySet> node_ptn = all_collect_vectors(loc_nodes) ;
      std::vector<entitySet> cell_ptn = all_collect_vectors(loc_cells) ;
      entitySet entities_accessed = Loci::MapRepP(tmp_face2node.Rep())->
        image(loc_faces) ;
      entitySet remap_out = entities_accessed - loc_nodes ;
      Loci::storeRepP remap_node_sp = Loci::MapRepP(remap_nodes.Rep())->
        expand(remap_out, node_ptn) ;
      Map tmp_remap_nodes(remap_node_sp) ; 
      entities_accessed = Loci::MapRepP(tmp_cl.Rep())->image(loc_faces) ;
      remap_out = entities_accessed - loc_cells ;
      entities_accessed = Loci::MapRepP(tmp_cr.Rep())->image(loc_faces) ;
      entities_accessed &= interval(0, Loci::UNIVERSE_MAX) ;
      remap_out += entities_accessed - loc_cells ;
      Loci::storeRepP remap_cell_sp=Loci::MapRepP(remap_cells.Rep())->
        expand(remap_out, cell_ptn) ;
      Map tmp_remap_cells(remap_cell_sp) ;
      ei = faces.begin() ;
      FORALL(loc_faces, fc) {
	new_count[*ei] = tmp_face2node[fc].size() ;
	cl[*ei] = tmp_remap_cells[tmp_cl[fc]] ;
	if(tmp_cr[fc] > 0)
	  cr[*ei] = tmp_remap_cells[tmp_cr[fc]] ;
	else
	  cr[*ei] = tmp_cr[fc] ;
	++ei ;
      } ENDFORALL ;
      
      FORALL(loc_nodes, nd) {
	pos[tmp_remap_nodes[nd]] = t_pos[nd] ; 
      } ENDFORALL ;
      face2node.allocate(new_count) ;
      ei = faces.begin() ;
      FORALL(loc_faces, fc) {
	int in = 0 ; 
	for(size_t ni = 0; ni != tmp_face2node[fc].size(); ++ni) {
	  face2node[*ei][in] = tmp_remap_nodes[tmp_face2node[fc][ni]] ;
	  ++in ;
	}
	++ei ;
      } ENDFORALL ;
      
      cri = Loci::MapRepP(cr.Rep())->image(faces) ;
      tmp_orig_boundaries =  cri & interval(Loci::UNIVERSE_MIN,-1) ;
      entitySet tmp_set = Loci::all_collect_entitySet(tmp_orig_boundaries) ;
      store<name_var> new_boundary_names ;
      new_boundary_names.allocate(tmp_set) ;
      Loci::debugout << " orig_boundaries = " << orig_boundaries << endl ;
      if(Loci::MPI_rank == 0)
	cout << "new boundaries identified as:" ;
      FORALL(tmp_set, bc) {
	char buf[512] ;
	sprintf(buf,"BC_%d",-bc) ;
	new_boundary_names[bc].name = buf ;
	if(Loci::MPI_rank == 0 )
	  cout << " " << new_boundary_names[bc].name ;
      } ENDFORALL ;
      if(Loci::MPI_rank == 0)
	cout << endl ;
      vset = all_collect_vectors(tmp_orig_boundaries) ;
      for(int i = 0; i < Loci::MPI_processes; ++i) {
	new_init_ptn[i] += vset[i] ; 
//Loci::debugout << " init_ptn[ " <<i << "] = " << new_init_ptn[i] << endl ;
      }
      param<int> min_node ;
      *min_node = max_alloc ;
      facts.create_fact("min_node", min_node) ; 
      facts.create_fact("cl", cl) ;
      facts.create_fact("cr", cr) ;
      facts.create_fact("pos", pos) ;
      facts.create_fact("face2node",face2node) ;
      facts.create_fact("boundary_names", new_boundary_names) ;
      constraint first_order_bc ;
      *first_order_bc = EMPTY ;
      facts.create_fact("first_order_bc",first_order_bc) ;
      if(Loci::MPI_rank == 0 ) {
	cout <<" All the procs finished reading the files " << endl ;
	xdr_destroy(&xdr_handle) ;
	fclose(FP) ;
      }
    }
    else {
      entitySet nodes = facts.get_allocation(npnts) ; 
      entitySet faces = facts.get_allocation(nfaces) ;
      Loci::debugout << "nodes = " << nodes << endl ;
      Loci::debugout << "faces = " << faces << endl ;
      entitySet cri = Loci::MapRepP(tmp_cr.Rep())->image(faces) ;
      
      entitySet tmp_set =  cri & interval(Loci::UNIVERSE_MIN,-1) ;
      multiMap face2node ;
      face2node.allocate(local_count) ;
      for(entitySet::const_iterator ei = faces.begin(); ei != faces.end(); ++ei) 
	for(int i = 0; i < local_count[*ei]; ++i) 
	  xdr_int(&xdr_handle, &face2node[*ei][i]) ;
      
      store<name_var> boundary_names ;
      boundary_names.allocate(tmp_set) ;
      cout << " boundaries identified as:" ;
      FORALL(tmp_set, bc) {
	char buf[512] ;
	sprintf(buf,"BC_%d", -bc) ;
	boundary_names[bc].name = buf ;
	if(Loci::MPI_rank == 0)
	  cout << " " << boundary_names[bc].name ;
      } ENDFORALL ;
      Map cl, cr ;
      cl.allocate(faces) ;
      cr.allocate(faces) ;
      FORALL(faces, fi) {
	cl[fi] = tmp_cl[fi] ;
	cr[fi] = tmp_cr[fi] ;
      }ENDFORALL ;
      
      store<vect3d> pos ;
      pos.allocate(nodes) ;
      FORALL(nodes, ni) {
	pos[ni] = t_pos[ni] ;
      } ENDFORALL ;
      param<int> min_node ;
      *min_node = 0 ;
      facts.create_fact("min_node", min_node) ;
      facts.create_fact("cl", cl) ;
      facts.create_fact("cr", cr) ;
      facts.create_fact("pos", pos) ;
      facts.create_fact("face2node",face2node) ;
      facts.create_fact("boundary_names", boundary_names) ;
      constraint first_order_bc ;
      *first_order_bc = EMPTY ;
      facts.create_fact("first_order_bc",first_order_bc) ;
      xdr_destroy(&xdr_handle) ;
      fclose(FP) ;
      cout << "\n Done with reading the file " << endl ;

    }
  }
}
