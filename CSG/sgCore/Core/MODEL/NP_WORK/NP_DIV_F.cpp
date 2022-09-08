#include "../../sg.h"

/*=== Global variables */
NP_STR_LIST c_list_str;
short         c_num_np = -32767;
/*--------------------------------------------------*/

/**********************************************************
* my internal function
*/
//-------------------------------------------------------->
typedef struct{
  D_POINT point; //intersect point
  short edge;     //edge point of intersection lie
}INTSECT_POINT;
typedef INTSECT_POINT * lpINTSECT_POINT;
//-------------------------------------------------------->
static short  found_first_point   ( lpNPW np_big, short *, short *number_of_point );
static short  found_second_point  ( lpNPW np, short, short number_of_point, sgFloat );
static short  contor_orient       ( lpNPW np );
static BOOL bild_line           ( lpNPW np, short *first, short, short *second );
static BOOL analis_array        ( lpNPW np, short *first, short *second, short edge, short sign[3] );
static BOOL bild_all_edge       ( lpNPW np, short first, short second );
static BOOL gabarit_test        ( lpNPW np, short first, short second,  short beg, short end );
static BOOL bild_intersect_edge ( lpNPW np, short first, short second, short j_b, short j_e, short place);
static void bild_own_intersect  ( lpNPW np, short *first, short second, short j );
static BOOL sorting_intersect   ( lpNPW np, short current, lpINTSECT_POINT list,
                                  short number_of_intersect );
static short  determ_intersection ( lpNPW np, short first, short second, short countour,
                                  lpINTSECT_POINT list1, lpINTSECT_POINT list2);
static BOOL calculate_intersection( lpNPW np, short p1, short p2,
                                     short p3, short p4, lpD_POINT p5 );
static short  found_neighbour_point( lpNPW np, short p1, short p2, lpD_POINT n );
static BOOL put_internal_edge    ( lpNPW np );
static BOOL exist_control        ( short point );
static BOOL insert_list          ( short place, short first, short second );
static short  def_loop             ( lpNPW np, short v );
static void def_edge             ( lpNPW np, short point, short *edge1, short *edge2 );
static short  control_face         ( lpNPW np );
static sgFloat sq                   ( lpNPW np, short A, short B, short F );
//-------------------------------------------------------->
short orient, place, face;
short number_of_intersect, *intersect_list; //inters. points
#define eps_d2 (eps_d * eps_d)
#define int_num 50
/**********************************************************
* subdivide big np on two or more small one
*/
BOOL sub_division( lpNPW np_big ){
  short    first, second, number_of_point, first_edge, i;
  sgFloat step[5] = {2, 3, 1.5, 4, 4/3};

  if( (intersect_list = (short*)SGMalloc( int_num*sizeof(short) ) ) == NULL )
    return FALSE;

//face number
  face  = 1;
  place = 0;
  if( !np_cplane_face( np_big, face ) ) goto err;

  while(1){
//determ orientation of external countour of face
    orient =  contor_orient( np_big );

//found first point on external countour and number of point on ext. count.
    first  = found_first_point( np_big, &first_edge, &number_of_point );
    for( i=0; i<5; i++ ){
//found second
      second = found_second_point( np_big, first_edge, number_of_point, step[i]);
/*
      tb_setcolor(IDXYELLOW);
      tb_put_otr(&np_big->v[first], &np_big->v[second]);
      put_message( PAUSE_MESSAGE, NULL, 1 );
      tb_put_otr(&np_big->v[first], &np_big->v[second]);
      put_message( PAUSE_MESSAGE, NULL, 1 );
*/
//bild line between first point and second
      if( bild_line( np_big, &first, first_edge,  &second )) break;
      else if( i >= 4 ) goto err;
    }
//bild intersection internal countours by line between first point and second
    if( !bild_all_edge( np_big, first, second ) ) goto err;
/*-
    for( i=0; i<number_of_intersect; i +=2 )
      tb_put_otr( &np_big->v[intersect_list[i]],
                  &np_big->v[intersect_list[i+1]]);
    put_message( PAUSE_MESSAGE, NULL, 1 );
*/
//bild DYMMY edges
    if( !put_internal_edge( np_big ) )  goto err;

//analisis of loops and faces
    if ( !np_fa( np_big, face ) )  goto err;

//control wether all faces < çó
    if( ( face = control_face( np_big ) ) == 0 ) break;
  }

  SGFree(intersect_list);

  if ( !np_del_sv(&c_list_str, &c_num_np, np_big) ) return FALSE;

  return TRUE;
err:
  np_handler_err(NP_DIV_FACE);
  SGFree(intersect_list);
  return  FALSE;
}

/**********************************************************
* found first point lieing  weight center
*/
static short found_first_point( lpNPW np, short *edge_first,
                              short *number_of_point ){
  short     point, point1, edge, first;
  sgFloat  distance, d;
  D_POINT v_center;

  edge  = np->c[ np->f[face].fc ].fe;
  point = BE( edge, np );
  (*number_of_point) = 1; // number of points leing on countor
  v_center.x = np->v[point].x;
  v_center.y = np->v[point].y;
  v_center.z = np->v[point].z;

//found weight center

//  point0 = point;
  do{
    edge   = SL( edge, np );
    point1 = BE( edge, np );
    (*number_of_point)++;
    v_center.x = v_center.x + np->v[point1].x;
    v_center.y = v_center.y + np->v[point1].y;
    v_center.z = v_center.z + np->v[point1].z;
  }while( EE( edge, np ) != point );
  v_center.x = v_center.x/(*number_of_point);
  v_center.y = v_center.y/(*number_of_point);
  v_center.z = v_center.z/(*number_of_point);

// found the first point
  *edge_first = edge  = np->c[ np->f[face].fc ].fe;
  first  = point = BE( edge, np );
  distance = dpoint_distance( &v_center, &np->v[point] );
  do{
    point1 = BE( edge, np );
    if( distance > ( d=dpoint_distance( &v_center, &np->v[point1] ) ) ){
      distance    = d;
      first       = point1;
      *edge_first = edge;
    }
    edge = SL( edge, np );
  }while( EE( edge, np ) != point );
  return( first );
}

/**********************************************************
* found second point lieing on external countour
*/
static short found_second_point( lpNPW np, short first_edge, short number_of_point,
                                sgFloat step){
  short  j=0, second;

//found second point
  while(1){
    second = EE( first_edge, np );
    j++;
    if( j >= (short)((sgFloat)number_of_point/step) ) return( second );
    first_edge = SL( first_edge, np );
  }
}

/**********************************************************
* bild convex poligon by changing first or second point
*/
static BOOL bild_line( lpNPW np, short *first,  short first_edge,
                                 short *second ){
  short     p_old, p_new, edge, sign[3], edge1, edge2, edge3, edge4;
  sgFloat  S_new, S_old;
  INTSECT_POINT *tmp_int;
  short     i, tmp_num, step=0;
  short   sign_old, sign_new;

while(1){
  if( (tmp_int = (INTSECT_POINT*)SGMalloc( int_num*sizeof(INTSECT_POINT) ) ) == NULL ) 
	  return FALSE;
// not enoughf memory??????????????????????????????????

//found intersection points for intersec line ( first, second) and countour
  edge = first_edge;
  p_old = *first;
  edge  = SL( edge, np );
  tmp_num=0;
  do{
    p_new = BE( edge, np );
    if( p_new != *first && p_new != *second &&
        p_old != *first && p_old != *second)
          if( gabarit_test( np, *first, *second, p_new, p_old) )
//            if( sq( np, *first, *second, p_new )*
//                sq( np, *first, *second, p_old ) < 0 ){
            if( sg(sq( np, *first, *second, p_new ), eps_d2) *
                sg(sq( np, *first, *second, p_old ), eps_d2) < 0 ){
              if( calculate_intersection( np, *first, *second, p_new,
                                    p_old, &tmp_int[tmp_num].point )){
                tmp_int[tmp_num++].edge = edge;
                if( tmp_num >= int_num ){
// too match intersection points??????????????????????????????????
				  SGFree(tmp_int);
                  return FALSE;
                }
              }
            }
    edge = SL( edge, np );
    p_old = p_new;
  }while( EE( edge, np ) != *first );

  if( tmp_num == 0 && step > 0 ) 
  {
	  SGFree(tmp_int);
	  return TRUE;
  }

  if( tmp_num > 0 ){
//sorting intersection with internal countours by distance from first point
    sorting_intersect( np, *first, tmp_int, tmp_num );

//search neib. point on countour
    for( i=0; i<tmp_num; i++)
      intersect_list[i] = found_neighbour_point( np, BE(tmp_int[i].edge, np),
                                      EE(tmp_int[i].edge, np), &tmp_int[i].point );
  }
  number_of_intersect = tmp_num;
  SGFree(tmp_int);

  memset(sign, 0, sizeof(sign));
  edge = first_edge;
  p_new = EE( edge, np );
  S_old = orient*sq( np, *first, *second, p_new );
  sign[0] = sign_old = sg(S_old, eps_d2);
  i=1;
  do{
    S_new = orient*sq( np, *first, *second, p_new );
    sign_new = sg(S_new, eps_d2);
    if (sign_old * sign_new <= 0) {
      sign[i++] = sign_new;
      if( i >= 3 ) break;
    }
    edge = SL( edge, np );
    p_new = EE( edge, np );
//    S_old = S_new;
    sign_old = sign_new;
  }while( p_new != *second );
//------------------------------------------------------------>>>>>
  if( analis_array( np, first, second, edge, sign ) ){
    step++;
    edge1 = 1; edge2 = 2; edge3 = 3; edge4 = 4;
    def_edge( np, *first, &edge1, &edge2 );
    def_edge( np, *second, &edge3, &edge4 );
    if( edge1 == edge3 || edge1 == edge4 || edge2 == edge3 || edge2 == edge4 ) return FALSE;
  }
  else return FALSE;
}
}

/**********************************************************
* analisis of resulting array for square chanching
*/
static BOOL analis_array( lpNPW np, short *first, short *second, short edge, short sign[3] ){

  if(number_of_intersect == 0){
    edge = SL( edge, np );
    do{
      if( orient*sg(sq( np, *first, *second, EE( edge, np )), eps_d2) *
          sign[0] < 0 )
          return TRUE; // lie inside
      edge = SL( edge, np );
    }while( EE(edge, np) != *first );
    return FALSE; // lie outside;
  }else{
    if(sign[0]>0 && sign[1]==0 && sign[2]>0 ) return FALSE;
    if(sign[0] < 0 ){
      *second = intersect_list[0];
      return TRUE;
    }
    if( sign[0] > 0 ){
      *first = intersect_list[0];
      if( number_of_intersect > 1 ) *second = intersect_list[1];
      return TRUE;
    }
  }
  return TRUE;
}

/**********************************************************
* bild intersection with edges of all internal countours first time
*/
static BOOL bild_all_edge( lpNPW np, short first, short second ){
  short     i, j, tmp_num=0;
  INTSECT_POINT *tmp_list;

  if( (tmp_list = (INTSECT_POINT*)SGMalloc( int_num*sizeof(INTSECT_POINT) ) ) == NULL ){
//memory??????????????????????????????????
    return FALSE;
  }
  for(i=0; i<int_num; i++ ) tmp_list[i].edge=0;

//bild intersection with all countours
  i = np->f[face].fc;
  do{
    if( i != 1 )
      if( ( determ_intersection( np, first, second, i,
            &tmp_list[tmp_num], &tmp_list[tmp_num+1] ) ) !=0 )  tmp_num += 2;
    i = np->c[ i ].nc;
  }while( i != 0 );

//sorting intersection with internal countours by distance from first point
  sorting_intersect( np, first, tmp_list, tmp_num );

//search neib. point on countour
 for( i=0; i<tmp_num; i++){
   intersect_list[i+1] = found_neighbour_point( np, BE(tmp_list[i].edge, np),
                         EE(tmp_list[i].edge, np), &tmp_list[i].point );
   if( intersect_list[i+1] == intersect_list[i] && i>0 ){
     intersect_list[i+1] = ( intersect_list[i+1] == BE(tmp_list[i].edge,np))?
       (EE(tmp_list[i].edge, np)) : (BE(tmp_list[i].edge, np));
   }
 }
//test on
 for( i=2; i<tmp_num; i += 2 ){
    if( def_loop( np, intersect_list[i] ) == def_loop( np, intersect_list[i+1] )){
      for( j=i; j<tmp_num; j++ ) intersect_list[j] = intersect_list[j+2];
      tmp_num -= 2;
      i -= 2;
    }
 }
 number_of_intersect = tmp_num+1;

//add first and second point
 intersect_list[0]                     = first;
 intersect_list[number_of_intersect++] = second;

 SGFree(tmp_list);
 return TRUE;
}

/**********************************************************
* determ wether intersection with countor exist
*/
static short determ_intersection( lpNPW np, short first, short second, short countour,
                                lpINTSECT_POINT list1, lpINTSECT_POINT list2){
  short     edge, point, point_b, point_e;
  INTSECT_POINT *tmp_int;
  short     tmp_num=0;

  if( (tmp_int = (INTSECT_POINT*)SGMalloc( int_num*sizeof(INTSECT_POINT) ) ) == NULL ) return 0;
// not enoughf memory??????????????????????????????????

  edge  = np->c[countour].fe;
  point = point_b = BE( edge, np );
  do{
    point_e = EE( edge, np );
    if( gabarit_test( np, first, second, point_e, point_b) )
      if( sg(sq( np, first, second, point_e ), eps_d2) *
          sg(sq( np, first, second, point_b ), eps_d2) < 0 ){
        if( calculate_intersection( np, first, second, point_e,
                                point_b, &tmp_int[tmp_num].point )){
          tmp_int[tmp_num++].edge = edge;
          if( tmp_num >= int_num ){
// too match intersection points??????????????????????????????????
              return FALSE;
          }
        }
      }
    edge = SL( edge, np );
    point_b = point_e;
  }while( BE( edge, np ) != point );
  if( tmp_num > 0 ){
//sorting intersection with internal countours by distance from first point
    sorting_intersect( np, first, tmp_int, tmp_num );
    list1->point.x = tmp_int[0].point.x;  list2->point.x = tmp_int[tmp_num-1].point.x;
    list1->point.y = tmp_int[0].point.y;  list2->point.y = tmp_int[tmp_num-1].point.y;
    list1->point.z = tmp_int[0].point.z;  list2->point.z = tmp_int[tmp_num-1].point.z;
    list1->edge = tmp_int[0].edge;        list2->edge = tmp_int[tmp_num-1].edge;
  }
  SGFree(tmp_int);
  return( tmp_num );
}

/**********************************************************
* gabarit test
*/
static BOOL g_test( sgFloat first, sgFloat second, sgFloat beg, sgFloat end ){
sgFloat max_i, min_i, max_p, min_p;

  max_i = max( first, second );
  min_i = min( first, second );
  max_p = max( beg, end );
  min_p = min( beg, end );
  if( min_i <= min_p && min_p <= max_i ||
      min_i <= max_p && max_p <= max_i ) return TRUE;
  else return FALSE;
}
static BOOL gabarit_test(  lpNPW np, short first, short second,
                           short beg, short end ){
  if( g_test( np->v[first].x, np->v[second].x, np->v[beg].x, np->v[end].x)||
      g_test( np->v[first].y, np->v[second].y, np->v[beg].y, np->v[end].y)||
      g_test( np->v[first].z, np->v[second].z, np->v[beg].z, np->v[end].z) )
        return TRUE;
  return FALSE;
}


/**********************************************************
* calculate intersection of two line (p1, p2) and (p3, p4)
* n_point - point of intersection
*/
static BOOL calculate_intersection( lpNPW np, short p1, short p2,
                                    short p3, short p4, lpD_POINT n_point ){
  lpDA_POINT vector1/*nb =NULL*/, vector2/*nb =NULL*/;
  lpDA_POINT point1, point2;
  short        i, j, n, n1;
  sgFloat     t1, t2, m[3];

  point1  = (lpDA_POINT)(&np->v[p1]);
  if( (vector1 = (DA_POINT*)SGMalloc( sizeof(DA_POINT) ) ) == NULL ) return FALSE;
  vector1[0][0] = np->v[p2].x - point1[0][0];
  vector1[0][1] = np->v[p2].y - point1[0][1];
  vector1[0][2] = np->v[p2].z - point1[0][2];

  point2  = (lpDA_POINT)(&np->v[p3]);
  if( (vector2 = (DA_POINT*)SGMalloc( sizeof(DA_POINT) ) ) == NULL ) goto err;
  vector2[0][0] = np->v[p4].x - point2[0][0];
  vector2[0][1] = np->v[p4].y - point2[0][1];
  vector2[0][2] = np->v[p4].z - point2[0][2];

  if( ( fabs(vector1[0][0]) < eps_d ) &&
      ( fabs(vector1[0][1]) < eps_d ) &&
      ( fabs(vector1[0][2]) < eps_d ) ) goto err; //intersection exsepts

  if( ( fabs(vector2[0][0]) < eps_d ) &&
      ( fabs(vector2[0][1]) < eps_d ) &&
      ( fabs(vector2[0][2]) < eps_d ) ) goto err; //intersection exsepts

  for( j=-1, i=0; i<3; i++ ){
    m[i] = vector1[0][ (i+2)%3 ]*vector2[0][(i+1)%3] -
           vector1[0][ (i+1)%3 ]*vector2[0][(i+2)%3];
    if( fabs( m[i] ) > eps_n ) j=i;
  }
  if( j < 0 ) goto err; //intersection exsepts

  t2 =  ( vector1[0][n = (j+2)%3]*( point1[0][ n1= (j+1)%3 ] -
                                    point2[0][n1] ) +
          vector1[0][n1]*( point2[0][n] - point1[0][n] ) )/m[j];
  if( t2 >= 1.+eps_d || t2 <= -eps_d ) goto err;
//ò°≠ if( t2 > 1 || t2 < 0 ) goto err;

  for( i=0; i<3; i++ )
    if( fabs( vector1[0][i] ) > eps_d && fabs( vector1[0][i] ) > eps_d ) j=i;
  t1 = ( point2[0][j] + vector2[0][j]*t2 - point1[0][j] )/vector1[0][j];
  if( t1 >= 1.+eps_d || t1 <= -eps_d ) goto err;
//ò°≠ if( t1 > 1 || t1 < 0 ) goto err;

  n_point->x = point2[0][0] + vector2[0][0]*t2;
  n_point->y = point2[0][1] + vector2[0][1]*t2;
  n_point->z = point2[0][2] + vector2[0][2]*t2;

  SGFree(vector1);
  SGFree(vector2);
  return TRUE;
err:
  if( vector1 != NULL) SGFree(vector1);
  if( vector2 != NULL) SGFree(vector2);
  return FALSE;
}

/**********************************************************
* found countour point lieing  point
*/
static short found_neighbour_point( lpNPW np, short p1, short p2,
                                  lpD_POINT n_point ){
  short  first;

  first = ( dpoint_distance( n_point, &np->v[p1] ) >
            dpoint_distance( n_point, &np->v[p2] ) ) ? p2 : p1;
  return( first );
}

/**********************************************************
* sorting egde og intersection by distance from point current
*/
static BOOL sorting_intersect( lpNPW np, short current,
                               lpINTSECT_POINT list, short number_of_intersect ){
  short     i, j, min, jl, R_edge;
  sgFloat  R_work;
  sgFloat  *work_list;
  D_POINT R_point;

  if( ( work_list = (sgFloat*)SGMalloc(int_num*sizeof(sgFloat))) == NULL ) return FALSE;

//calculate distance
  for( i=0; i<number_of_intersect; i++  )
    work_list[i] = dpoint_distance( &np->v[current], &list[i].point );

//sortig by distance
  for( i=0; i<number_of_intersect; i++ ){
    for( min = 0, R_work = work_list[i], j=i+1; j<number_of_intersect; j++ ){
      if( work_list[j] < R_work ){
        R_work  = work_list[j];
        R_point = list[j].point;
        R_edge  = list[j].edge;
        min = 1;
        jl = j;
      }
    }
    if( min ){
      work_list[jl]  = work_list[i];
      list[jl].point = list[i].point;
      list[jl].edge  = list[i].edge;
      work_list[i]   = R_work;
      list[i].point  = R_point;
      list[i].edge   = R_edge;
    }
  }
  SGFree(work_list);
  return TRUE;
}

/**********************************************************
* put DUMMY edges
*/
static BOOL put_internal_edge( lpNPW np ){
  short i=0, edge, last_edge1, last_edge2;
  short loop1, loop2, countour_b, countour_e;

  while(1){
//determ countour point lie on
    countour_b = def_loop( np, intersect_list[i] );
    countour_e = def_loop( np, intersect_list[i+1] );

//bild intersection own countours by line between current_b and current_e
    bild_own_intersect( np, &intersect_list[i], intersect_list[i+1], countour_b );
    bild_own_intersect( np, &intersect_list[i+1], intersect_list[i], countour_e );

//bild intersection internal countours by line between current_b
//point and current_e
    loop1 = number_of_intersect;
    if( !bild_intersect_edge( np, intersect_list[i], intersect_list[i+1],
                         countour_b, countour_e, i)) return FALSE;
    if( loop1 == number_of_intersect ){
// if number of point is"t changed take next pare
      i += 2;
      if( number_of_intersect - i <= 0 ) break;
    }
  }

//put DUMMY edges to np
  for( i=0; i<number_of_intersect; i += 2 ){
    if( !(edge = np_new_edge( np, intersect_list[i], intersect_list[i+1] ) )) return FALSE;
    np->efr[edge].el = ST_TD_DUMMY;
//if edge not exist in çó
    if( edge > 0 ){
      np->efc[edge].fp = np->efc[edge].fm = face;
      np->efc[edge].ep = -orient*edge;
      np->efc[edge].em =  orient*edge;
//define next edge
      loop1 = np_def_loop( np, face, orient*edge, &last_edge1 );
      loop2 = np_def_loop( np, face,-orient*edge, &last_edge2 );
      if( loop1 != 0 && loop2 != 0 ) {
//insert edge with labels
        np_insert_edge( np,  orient*edge, last_edge1 );
        np_insert_edge( np, -orient*edge, last_edge2 );
//divide existed countours
        if( !np_an_loops( np, face, loop1, loop2, edge ) ) return FALSE;
      }
    }
  }
  return TRUE;
}

/**********************************************************
* define countour point lie on
*/
static short def_loop( lpNPW np, short v ){
  short point_t, point_b, edge, countour;

  countour = np->f[face].fc;
  do{
    edge    = np->c[countour].fe;
    point_b = point_t = BE( edge, np );
    do{
      if( point_t == v ) return countour;
      edge    = SL( edge, np );
      point_t = BE( edge, np );
    }while ( point_t != point_b);
    countour = np->c[countour].nc;
  }while (countour != 0);
  return 0;
}

/**********************************************************
* define edge point lie on
*/
static void def_edge( lpNPW np, short point, short *edge1, short *edge2 ){
  short edge, point1;
  edge  = np->c[ np->f[face].fc ].fe;
  point1 = EE( edge, np );
  do{
    if( point == point1 ){
      *edge1 = edge;
      *edge2 = SL(edge, np);
      return;
    }
    edge   = SL( edge, np );
    point1 = EE( edge, np );
  }while( EE( edge, np ) != point );
}
/**********************************************************
* bild intersection with edges of countour point lie on (i)
*/
static void bild_own_intersect( lpNPW np, short *first, short second, short c_f ){
INTSECT_POINT list1, list2;

  if( ( determ_intersection( np, *first, second, c_f, &list1, &list2) ) != 0 )
    *first = found_neighbour_point( np, BE(list1.edge, np), EE(list1.edge,np),
                                            &list1.point );
}

/**********************************************************
* bild intersection with edges of internal countours
*/
static BOOL bild_intersect_edge( lpNPW np, short first, short second, short j_b,
                                 short j_e, short place ){
  short     i;
  INTSECT_POINT list1, list2;

  i = np->f[face].fc;
  do{
    if( i != j_b && i != j_e ){
      if( ( determ_intersection( np, first, second, i, &list1, &list2 ) ) !=0 ){
//found neighbour points for intersect points
        list1.edge = found_neighbour_point( np, BE(list1.edge, np),
                     EE(list1.edge, np), &list1.point );
        list2.edge = found_neighbour_point( np, BE(list2.edge, np),
                     EE(list2.edge, np), &list2.point );

        if( !(exist_control( list1.edge )) && !(exist_control( list2.edge )) )
          if(!insert_list( place, list1.edge, list2.edge ) ) return FALSE;
      }
    }
    i = np->c[ i ].nc;
  }while( i != 0 );
  return TRUE;
}

/**********************************************************
* control wether point already exist into list
*/
static BOOL exist_control( short point ){
  short i;

  for( i=0; i<number_of_intersect; i++ )
    if( intersect_list[i] == point) return TRUE;
  return FALSE;
}

/**********************************************************
* insert points of intersection into list
*/
static BOOL insert_list( short place, short first, short second ){
  short i;

  if( number_of_intersect+2 > int_num ) return FALSE;
  for( i= number_of_intersect-1; i>=place+1; i-- )
    intersect_list[i+2] = intersect_list[i];
  intersect_list[place+1] = first;
  intersect_list[place+2] = second;
  number_of_intersect += 2;
  return TRUE;
}

/**********************************************************
* wether exist face > çó
*/
static short control_face( lpNPW np ){
  short i, number, edge, first_c, first_v;
  for( i=1; i<=np->nof; i++ ){
    first_c = np->f[i].fc;
    number = 0;
    do{
      edge    = np->c[first_c].fe;
      first_v = BE( edge, np );
      do{
        number++;
        edge = SL( edge, np );
      }while( first_v != EE( edge, np ) );
      number++;
      first_c = np->c[first_c].nc;
    }while( first_c != 0 );
    if( number > MAXNOV || number > MAXNOE ) return (i);
  }
  return 0;
}

/**********************************************************
* determ countour orientation
*/
static short contor_orient( lpNPW np ){
  sgFloat S=0;
  short    vertex, edge;

  edge   = np->c[ np->f[face].fc ].fe;
  vertex = BE( edge, np );
  do{
    edge = SL( edge, np );
    S = S + sq( np, vertex, BE(edge, np), EE(edge, np) );
  }while( EE( edge, np ) != vertex );
  if( S >= 0 ) return (  1 );
  else         return ( -1 );
}

/**********************************************************
* calculate oriented square
*/
static sgFloat sq( lpNPW np, short A, short B, short F ){
  if( fabs(np->v[A].z ) < eps_n && fabs(np->v[B].z ) < eps_n &&
      fabs(np->v[F].z ) < eps_n )
      return( np->v[A].x*( np->v[B].y - np->v[F].y ) +
              np->v[B].x*( np->v[F].y - np->v[A].y ) +
              np->v[F].x*( np->v[A].y - np->v[B].y ));
 else return( ( np->v[B].y - np->v[A].y )*
              ( np->v[B].z - np->v[F].z ) +
              ( np->v[B].x - np->v[A].x )*
              ( np->v[B].y - np->v[F].y ) +
              ( np->v[B].z - np->v[A].z )*
              ( np->v[B].x - np->v[F].x ) -
              ( np->v[B].y - np->v[A].y )*
              ( np->v[B].x - np->v[F].x ) -
              ( np->v[B].z - np->v[A].z )*
              ( np->v[B].y - np->v[F].y ) -
              ( np->v[B].x - np->v[A].x )*
              ( np->v[B].z - np->v[F].z ) );
}
