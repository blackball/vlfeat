/** @file   hikmeans.c
 ** @author Brian Fulkerson
 ** @author Andrea Vedaldi
 ** @brief  Heirachical Integer K-Means Clustering - Declaration
 **/

/** @file hikmeans.h
 ** @brief Hierachical integer K-Means clustering
 ** 
 ** Hierarichial integer K-Means clustering (HIKM) is a simple
 ** hierarchical version of integer K-Means (@ref ikmeans.h
 ** "IKM"). The algorithm recusrively apply integer K-means to create
 ** more refined partitions of the data.
 **
 ** Use the ::vl_hikm() function to partition the data and create a
 ** HIKM tree (and ::vl_hikm_delete() to dispose of it).  Use
 ** ::vl_hikm_push() to project new data down a HIKM tree.
 **
 ** @section hikm-tree HIKM tree
 **
 ** The HIKM tree is a simple structure, represented by ::VlHIKMTree
 ** and ::VlHIKMNode. The tree as a 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "hikmeans.h"

/** ------------------------------------------------------------------
 ** @internal
 ** @brief Copy a subset of the data to a buffer
 **
 ** @param data  Data.
 ** @param ids   Data labels.
 ** @param N     Number of indeces.
 ** @param M     Data dimensionality. 
 ** @param id    Label of data to copy.
 ** @param N2    Number of data copied (out).
 **
 ** @return a new buffer with a copy of the selected data.
 **/

vl_ikm_data*
vl_hikm_copy_subset (vl_ikm_data const * data, 
                     vl_uint * ids, 
                     int N, int M, 
                     vl_uint id, int *N2)
{
  int i ;
  int count = 0;

  /* count how many data with this label there are */
  for (i = 0 ; i < N ; i++)
    if (ids[i] == id)
      count ++ ;
  *N2 = count ;
  
  /* copy each datum to the buffer */
  {
    vl_ikm_data * new_data = vl_malloc (sizeof(vl_ikm_data) * M * count);    
    count = 0;
    for (i = 0 ; i < N ; i ++)
      if (ids[i] == id)
        {
          memcpy(new_data + count * M, 
                 data     + i     * M, 
                 sizeof(vl_ikm_data) * M);
          count ++ ;
        }    
    *N2 = count ;
    return new_data ;
  }  
}

/** ------------------------------------------------------------------
 ** @brief Compute hierarchical integer K-means clustering.
 **
 ** @param hikm   HIKM tree to fill.
 ** @param data   Data to cluster.
 ** @param N      Number of data points.
 ** @param K      Number of clusters for this node.
 ** @param depth  Tree depth.
 **
 ** @return a new HIKM node representing a sub-clustering.
 **/

static VlHIKMNode * 
xmeans (VlHIKMTree *tree, 
        vl_ikm_data const *data, 
        int N, int K, int height)
{
  VlHIKMNode *node = vl_malloc (sizeof(VlHIKMNode)) ;
  vl_uint     *ids = vl_malloc (sizeof(vl_uint) * N) ;

  node-> filter   = vl_ikm_new (tree -> method) ;    
  node-> children = (height == 1) ? 0 : vl_malloc (sizeof(VlHIKMNode*) * K) ;

  vl_ikm_set_max_niters (node->filter, tree->max_niters) ;
  vl_ikm_set_verbosity  (node->filter, tree->verb - 1  ) ;
  vl_ikm_init_rand_data (node->filter, data, tree->M, N, K) ;
  vl_ikm_train          (node->filter, data, N) ;
  vl_ikm_push           (node->filter, ids, data, N) ;
  
  /* recurse for each child */
  if (height > 1) {
    int k ;
    for (k = 0 ; k < K ; k ++) {
      int partition_N ;
      int partition_K ;
      vl_ikm_data *partition ;
      
      partition = vl_hikm_copy_subset
        (data, ids, N, tree->M, k, &partition_N) ;
      
      partition_K = VL_MIN (K, partition_N) ;
      
      node->children [k] = xmeans
        (tree, partition, partition_N, partition_K, height - 1) ;
      
      vl_free (partition) ;

      if (tree->verb > tree->depth - height) {
        VL_PRINTF("hikmeans: branch at depth %d: %6.1f %% completed\n", 
                  tree->depth - height,
                  (double) (k+1) / K * 100) ;
      }
    }
  }
  
  vl_free (ids) ;
  return node ;
}

/** ------------------------------------------------------------------
 ** @internal 
 ** @brief Delete node 
 **
 ** @param node to delete.
 **
 ** The function deletes recursively @a node and all its descendent.
 **/

static void 
xdelete (VlHIKMNode *node)
{
  if(node) {
    int k ;
    if (node->children) {
      for(k = 0 ; k < vl_ikm_get_K (node->filter) ; ++k)
        xdelete (node->children[k]) ;
      vl_free (node->children) ;
    }
    if (node->filter) 
      vl_ikm_delete (node->filter) ;
    vl_free(node);
  }
}

/** ------------------------------------------------------------------
 ** @brief New HIKM tree
 ** @param mehtod clustering method.
 ** @return new HIKM tree.
 **/

VlHIKMTree *
vl_hikm_new (int method)
{
  VlHIKMTree *f = vl_malloc (sizeof(VlHIKMTree)) ;
  f -> M          = 0 ;
  f -> K          = 0 ;
  f -> max_niters = 200 ;
  f -> method     = method ;
  f -> verb       = 0 ;
  f -> depth      = 0 ;
  f -> root       = 0 ;
  return f ;
}

/** ------------------------------------------------------------------
 ** @brief Delete HIKM tree
 ** @param f HIKM tree.
 **/

void
vl_hikm_delete (VlHIKMTree *f)
{
  if (f) {
    xdelete (f -> root) ;
    vl_free (f) ;
  }
}

/** ------------------------------------------------------------------
 ** @brief Initialize HIKM tree
 **
 ** @param f     HIKM tree.
 ** @param M     Data dimensionality.
 ** @param K     Number of clusters per node.
 ** @param depth Tree depth.
 **
 ** @return a new HIKM tree representing the clustering.
 **/

void
vl_hikm_init (VlHIKMTree *f, int M, int K, int depth)
{
  xdelete (f -> root) ;
  f -> root = 0;
  
  f -> M = M ;
  f -> K = K ;
  f -> depth = depth ;
}

/** ------------------------------------------------------------------
 ** @brief Train HIKM tree
 **
 ** @param data    Data to cluster.
 ** @param N       Number of data.
 **/ 

void
vl_hikm_train (VlHIKMTree *f, vl_ikm_data const *data, int N)
{
  f -> root  = xmeans (f, data, N, VL_MIN(f->K, N), f->depth) ;
}

/** ------------------------------------------------------------------
 ** @biref Project data down HIKM tree
 **
 ** @param hikm HIKM tree.
 ** @param data Data to project.
 ** @param N    Number of data.
 **
 ** @return a new vector with the data to node assignments.
 **/
void
vl_hikm_push (VlHIKMTree *f, vl_uint *asgn, vl_ikm_data const *data, int N)
{
  int i, d,
    M = vl_hikm_get_ndims (f),
    depth = vl_hikm_get_depth (f) ;
  
  /* for each datum */
  for(i = 0 ; i < N ; i++) {
    VlHIKMNode *node = f->root ;
    d = 0 ;      
    while (node) {
      /*
      vl_uint best = 
        vl_ikm_push_one (vl_ikm_get_centers (node->filter),  
                         data + i * M,
                         M,
                         vl_ikm_get_K (node->filter)) ;
      */
      
      vl_uint best ;
      vl_ikm_push (node->filter, &best, data + i * M, 1) ;
      
      asgn [i*depth + d] = best ;
      ++ d ;
      
      if (!node->children) break ;
      node = node->children [best] ;
    }
  }
}