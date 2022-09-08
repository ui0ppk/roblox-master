
#ifndef _UBLAS_EXT_H
#define _UBLAS_EXT_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/static_assert.hpp> 


//should we put this in a different namespace???
namespace boost { namespace numeric { namespace ublas {

	/* Matrix inversion routine.
	Uses lu_factorize and lu_substitute in uBLAS to invert a matrix */
	template<class T>
	bool invert_matrix (const matrix<T>& input, matrix<T>& inverse) 
	{
		typedef permutation_matrix<std::size_t> pmatrix;
		// create a working copy of the input
		matrix<T> A(input);
		// create a permutation matrix for the LU-factorization
		pmatrix pm(A.size1());

		// perform LU-factorization
		int res = lu_factorize(A,pm);
		if( res != 0 ) 
			return false;

		// create identity matrix of "inverse"
		inverse.assign(ublas::identity_matrix<T>(A.size1()));

		// backsubstitute to get the inverse
		lu_substitute(A, pm, inverse);

		return true;
	}

	// LU factorization with partial pivoting
    // keeps factorizing even singular matrices.
    // returns number of independent columns!
	template<class M, class PM, class IsZeroFunctor>
	typename M::size_type lu_factorize_singular (M &m, PM &pm, const IsZeroFunctor& iszero ) {
        typedef M matrix_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

#if BOOST_UBLAS_TYPE_CHECK
        matrix_type cm (m);
#endif
        //int singular = 0;
        size_type size1 = m.size1 ();
        size_type size2 = m.size2 ();
        size_type size = (std::min) (size1, size2);
		size_type columni = 0;
		size_type rowi = 0;
        for (; rowi < size && columni < size2; ++ rowi, ++ columni) {
            matrix_column<M> mci (column (m, columni));
            matrix_row<M> mri (row (m, rowi));
            size_type i_norm_inf = rowi + index_norm_inf (project (mci, range (rowi, size1)));
            BOOST_UBLAS_CHECK (i_norm_inf < size1, external_logic ());
            if (!iszero(m (i_norm_inf, columni)) ) {
                if (i_norm_inf != rowi) {
                    pm (rowi) = i_norm_inf;
                    row (m, i_norm_inf).swap (mri);
                } else {
                    BOOST_UBLAS_CHECK (pm (rowi) == i_norm_inf, external_logic ());
                }
                project (mci, range (rowi + 1, size1)) *= value_type (1) / m (rowi, columni);
            } else {
				// whole column is zero.
				// move to next column.
				rowi--; continue;
            }
            project (m, range (rowi + 1, size1), range (columni + 1, size2)).minus_assign (
                outer_prod (project (mci, range (rowi + 1, size1)),
                            project (mri, range (columni + 1, size2))));
        }
        return rowi;
    }

    template<class PM, class MV>
    BOOST_UBLAS_INLINE
    void unswap_rows (const PM &pm, MV &mv, vector_tag) {
        typedef typename PM::size_type size_type;
        typedef typename MV::value_type value_type;

        size_type size = pm.size ();
        for (size_type i = size-1; i != ~0; -- i) {
            if (i != pm (i))
                std::swap (mv (i), mv (pm (i)));
        }
    }
    template<class PM, class MV>
    BOOST_UBLAS_INLINE
    void unswap_rows (const PM &pm, MV &mv, matrix_tag) {
        typedef typename PM::size_type size_type;
        typedef typename MV::value_type value_type;

        size_type size = pm.size ();
        for (size_type i = size-1; i != ~0; -- i) {
            if (i != pm (i))
                row (mv, i).swap (row (mv, pm (i)));
        }
    }
    // Dispatcher
    template<class PM, class MV>
    BOOST_UBLAS_INLINE
    void unswap_rows (const PM &pm, MV &mv) {
        unswap_rows (pm, mv, typename MV::type_category ());
    }

}}} // namespace

#endif