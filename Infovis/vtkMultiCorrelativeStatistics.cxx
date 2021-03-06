#include "vtkMultiCorrelativeStatistics.h"
#include "vtkMultiCorrelativeStatisticsAssessFunctor.h"

#include "vtkDataObject.h"
#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/map>
#include <vtksys/stl/vector>
#include <vtksys/ios/sstream>

#define VTK_MULTICORRELATIVE_KEYCOLUMN1 "Column1"
#define VTK_MULTICORRELATIVE_KEYCOLUMN2 "Column2"
#define VTK_MULTICORRELATIVE_ENTRIESCOL "Entries"
#define VTK_MULTICORRELATIVE_AVERAGECOL "Mean"
#define VTK_MULTICORRELATIVE_COLUMNAMES "Column"

vtkCxxRevisionMacro(vtkMultiCorrelativeStatistics,"$Revision$");
vtkStandardNewMacro(vtkMultiCorrelativeStatistics);

// ----------------------------------------------------------------------
vtkMultiCorrelativeStatistics::vtkMultiCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues( 1 );
  this->AssessNames->SetValue( 0, "d^2" ); // Squared Mahalanobis distance
}

// ----------------------------------------------------------------------
vtkMultiCorrelativeStatistics::~vtkMultiCorrelativeStatistics()
{
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeInvertCholesky( vtksys_stl::vector<double*>& chol, vtksys_stl::vector<double>& inv )
{
  vtkIdType m = static_cast<vtkIdType>( chol.size() );
  inv.resize( m * ( m + 1 ) / 2 );

  vtkIdType i, j, k;
  for ( i = 0; i < m; ++ i )
    {
    vtkIdType rsi = ( i * ( i + 1 ) ) / 2; // start index of row i in inv.
    inv[rsi + i] = 1. / chol[i][i];
    for ( j = i; j > 0; )
      {
      inv[rsi + (-- j)] = 0.;
      for ( k = j; k < i; ++ k )
        {
        vtkIdType rsk = ( k * ( k + 1 ) ) / 2;
        inv[rsi + j] -= chol[k][i] * inv[rsk + j];
        }
      inv[rsi + j] *= inv[rsi + i]; 
      }
    }
  // The result, stored in \a inv as a lower-triangular, row-major matrix, is
  // the inverse of the Cholesky decomposition given as input (stored as a
  // rectangular, column-major matrix in \a chol). Note that the super-diagonal
  // entries of \a chol are not zero as you would expect... we just cleverly
  // don't reference them.
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeTransposeTriangular( vtksys_stl::vector<double>& a, vtkIdType m )
{
  vtksys_stl::vector<double> b( a.begin(), a.end() );
  double* bp = &b[0];
  vtkIdType i, j;
  a.clear();
  double* v;
  for ( i = 0; i < m; ++ i )
    {
    v = bp + ( i * ( i + 3 ) ) / 2; // pointer to i-th entry along diagonal (i.e., a(i,i)).
    for ( j = i; j < m; ++ j )
      {
      a.push_back( *v );
      v += ( j + 1 ); // move down one row
      }
    }

  // Now, if a had previously contained: [ A B C D E F G H I J ], representing the
  // lower triangular matrix: A          or the upper triangular: A B D G
  // (row-major order)        B C           (column-major order)    C E H
  //                          D E F                                   F I
  //                          G H I J                                   J
  // It now contains [ A B D G C E H F I J ], representing
  // upper triangular matrix : A B D G   or the lower triangular: A
  // (row-major order)           C E H      (column-major order)  B C
  //                               F I                            D E F
  //                                 J                            G H I J
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeAssessFunctor::operator () ( vtkVariantArray* result, vtkIdType row )
{
  vtkIdType m = static_cast<vtkIdType>( this->Columns.size() );
  vtkIdType i, j;
  this->Tuple = this->EmptyTuple; // initialize Tuple to 0.0
  double* x = &this->Tuple[0];
  double* y;
  double* ci = &this->Factor[0];
  double v;
  for ( i = 0; i < m; ++ i )
    {
    v = this->Columns[i]->GetTuple( row )[0] - this->Center[i];
    y = x + i;
    for ( j = i; j < m; ++ j, ++ ci, ++ y )
      {
      (*y) += (*ci) * v;
      }
    }
  double r = 0.;
  y = x;
  for ( i = 0; i < m; ++ i, ++ y )
    {
    r += (*y) * (*y);
    }

  result->SetNumberOfValues( 1 );
  // To report cumulance values instead of relative deviation, use this:
  // result->SetValue( 0, exp( -0.5 * r ) * pow( 0.5 * r, 0.5 * m - 2.0 ) * ( 0.5 * ( r + m ) - 1.0 ) / this->Normalization );
  result->SetValue( 0, r );
}

// ----------------------------------------------------------------------
int vtkMultiCorrelativeStatistics::FillInputPortInformation( int port, vtkInformation* info )
{
  int stat; // = this->Superclass::FillInputPortInformation( port, info );
  if ( port == INPUT_MODEL )
    {
    info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet" );
    info->Set( vtkAlgorithm::INPUT_IS_OPTIONAL(), 1 );
    stat = 1;
    }
  else
    {
    stat = this->Superclass::FillInputPortInformation( port, info );
    }
  return stat;
}

// ----------------------------------------------------------------------
int vtkMultiCorrelativeStatistics::FillOutputPortInformation( int port, vtkInformation* info )
{
  int stat = this->Superclass::FillOutputPortInformation( port, info );
  if ( port == OUTPUT_MODEL )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
    }
  return stat;
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Aggregate( vtkDataObjectCollection* inMetaColl,
                                               vtkDataObject* outMetaDO )
{
  // Verify that the output model is indeed contained in a multiblock data set
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  if ( ! outMeta ) 
    { 
    return; 
    } 

  // Get hold of the first model (data object) in the collection
  vtkCollectionSimpleIterator it;
  inMetaColl->InitTraversal( it );
  vtkDataObject *inMetaDO = inMetaColl->GetNextDataObject( it );

  // Verify that the first input model is indeed contained in a multiblock data set
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta ) 
    { 
    return; 
    }

  // Verify that the first covariance matrix is indeed contained in a table
  vtkTable* inCov = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
  if ( ! inCov )
    {
    return;
    }

  vtkIdType nRow = inCov->GetNumberOfRows();
  if ( ! nRow )
    {
    // No statistics were calculated.
    return;
    }

  // Use this first model to initialize the aggregated one
  vtkTable* outCov = vtkTable::New();
  outCov->DeepCopy( inCov );

  // Now, loop over all remaining models and update aggregated each time
  while ( ( inMetaDO = inMetaColl->GetNextDataObject( it ) ) )
    {
    // Verify that the current model is indeed contained in a multiblock data set
    inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
    if ( ! inMeta ) 
      { 
      return; 
      }

    // Verify that the current covariance matrix is indeed contained in a table
    inCov = vtkTable::SafeDownCast( inMeta->GetBlock( 0 ) );
    if ( ! inCov )
      {
      return;
      }

    if ( inCov->GetNumberOfRows() != nRow )
      {
      // Models do not match
      return;
      }

    // Iterate over all model rows
    int inN, outN;
    double muFactor = 0.;
    double covFactor = 0.;
    vtkstd::vector<double> inMu, outMu;
    int j = 0;
    int k = 0;
    for ( int r = 0; r < nRow; ++ r )
      {
      // Verify that variable names match each other
      if ( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN1 ) != outCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN1 )
           || inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN2 ) != outCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN2 ) )
        {
        // Models do not match
        return;
        }

      // Update each model parameter
      if ( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN1 ).ToString() == "Cardinality" )
        {
        // Cardinality
        inN = inCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToInt();
        outN = outCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToInt();
        int totN = inN + outN;
        outCov->SetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL, totN );
        muFactor = static_cast<double>( inN ) / totN;
        covFactor = static_cast<double>( inN ) * outN / totN;
        }
      else if ( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_KEYCOLUMN2 ).ToString() == "" )
        {
        // Mean
        inMu.push_back( inCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble() );
        outMu.push_back( outCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble() );
        outCov->SetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL, outMu.back() + ( inMu.back() - outMu.back() ) * muFactor );
        }
      else
        {
        // M XY
        double inCovEntry = inCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble();
        double outCovEntry = outCov->GetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL ).ToDouble();
        outCov->SetValueByName( r, VTK_MULTICORRELATIVE_ENTRIESCOL, inCovEntry + outCovEntry + ( inMu[j] - outMu[j] ) * ( inMu[k] - outMu[k] ) * covFactor );
        ++ k;
        if ( k > j )
          {
          ++ j;
          k = 0;
          }
        }
      }
    }
  
  // Replace covariance block of output model with updated one
  outMeta->SetBlock( 0, outCov );

  // Clean up
  outCov->Delete();

  return;
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Learn( vtkTable* inData, 
                                           vtkTable* vtkNotUsed( inParameters ),
                                           vtkDataObject* outMetaDO )
{
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  if ( ! outMeta )
    {
    return;
    }

  vtkTable* sparseCov = vtkTable::New();

  vtkStringArray* ocol1 = vtkStringArray::New();
  ocol1->SetName( VTK_MULTICORRELATIVE_KEYCOLUMN1 );
  sparseCov->AddColumn( ocol1 );
  ocol1->Delete();

  vtkStringArray* ocol2 = vtkStringArray::New();
  ocol2->SetName( VTK_MULTICORRELATIVE_KEYCOLUMN2 );
  sparseCov->AddColumn( ocol2 );
  ocol2->Delete();

  vtkDoubleArray* mucov = vtkDoubleArray::New();
  mucov->SetName( VTK_MULTICORRELATIVE_ENTRIESCOL );
  sparseCov->AddColumn( mucov );
  mucov->Delete();

  vtkIdType n = inData->GetNumberOfRows();
  if ( n <= 0 )
    {
    return;
    }

  if ( inData->GetNumberOfColumns() <= 0 )
    {
    return;
    }

  vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator reqIt;
  vtksys_stl::set<vtkStdString>::const_iterator colIt;
  vtksys_stl::set<vtksys_stl::pair<vtkStdString,vtkDataArray*> > allColumns;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType> colPairs;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType>::iterator cpIt;
  vtksys_stl::map<vtkStdString,vtkIdType> colNameToIdx;
  vtksys_stl::vector<vtkDataArray*> colPtrs;

  // Populate a vector with pointers to columns of interest (i.e., columns from the input dataset
  // which have some statistics requested) and create a map from column names into this vector.
  // The first step is to create a set so that the vector entries will be sorted by name.
  for ( reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++ reqIt )
    {
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      // Ignore invalid column names
      vtkDataArray* arr = vtkDataArray::SafeDownCast( inData->GetColumnByName( colIt->c_str() ) );
      if ( arr )
        {
        allColumns.insert( vtksys_stl::pair<vtkStdString,vtkDataArray*>( *colIt, arr ) );
        }
      }
    }
  // Now make a map from input column name to output column index (colNameToIdx):
  vtkIdType i = 0;
  vtkIdType m = static_cast<vtkIdType>( allColumns.size() );
  vtksys_stl::set<vtksys_stl::pair<vtkStdString,vtkDataArray*> >::const_iterator acIt;
  vtkStdString empty;
  ocol1->InsertNextValue( "Cardinality" );
  ocol2->InsertNextValue( empty );
  for ( acIt = allColumns.begin(); acIt != allColumns.end(); ++ acIt )
    {
    colNameToIdx[acIt->first] = i ++;
    colPtrs.push_back( acIt->second );
    ocol1->InsertNextValue( acIt->second->GetName() );
    ocol2->InsertNextValue( empty );
    }

  // Get a list of column pairs (across all requests) for which we'll compute sums of squares.
  // This keeps us from computing the same covariance entry multiple times if several requests
  // contain common pairs of columns.
  i = m;
  // For each request:
  for ( reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++ reqIt )
    {
    // For each column in the request:
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      vtksys_stl::map<vtkStdString,vtkIdType>::iterator idxIt = colNameToIdx.find( *colIt );
      // Ignore invalid column names
      if ( idxIt != colNameToIdx.end() )
        {
        vtkIdType colA = idxIt->second;
        vtkStdString colAName = idxIt->first;
        vtksys_stl::set<vtkStdString>::const_iterator colIt2;
        for ( colIt2 = colIt; colIt2 != reqIt->end(); ++ colIt2 )
          {
          idxIt = colNameToIdx.find( *colIt2 );
          // Ignore invalid column names
          if ( idxIt != colNameToIdx.end() )
            { // Note that other requests may have inserted this entry.
            vtksys_stl::pair<vtkIdType,vtkIdType> entry( colA, idxIt->second );
            if ( colPairs.find( entry ) == colPairs.end() )
              { // point to the offset in mucov (below) for this column-pair sum:
              //cout << "Pair (" << colAName.c_str() << ", " << idxIt->first.c_str() << "): " << colPairs[entry] << "\n";
              colPairs[entry] = -1;
              }
            }
          }
        }
      }
    }

  // Now insert the column pairs into ocol1 and ocol2 in the order in which they'll be evaluated.
  for ( cpIt = colPairs.begin(); cpIt != colPairs.end(); ++ cpIt )
    {
    cpIt->second = i ++;
    ocol1->InsertNextValue( colPtrs[cpIt->first.first]->GetName() );
    ocol2->InsertNextValue( colPtrs[cpIt->first.second]->GetName() );
    }

  // Now (finally!) compute the covariance and column sums.
  // This uses the on-line algorithms for computing centered
  // moments and covariances from Philippe's SAND2008-6212 report.
  double* x;
  vtksys_stl::vector<double> v( m, 0. ); // Values (v) for one observation
  //vtksys_stl::vector<double> mucov( m + colPairs.size(), 0. ); // mean (mu) followed by covariance (cov) values
  mucov->SetNumberOfTuples( 1 + m + colPairs.size() ); // sample size followed by mean (mu) followed by covariance (cov) values
  mucov->FillComponent( 0, 0. );
  double* rv = mucov->GetPointer( 0 );
  *rv = static_cast<double>( n );
  ++ rv; // skip Cardinality entry
  for ( i = 0; i < n; ++ i )
    {
    // First fetch column values
    for ( vtkIdType j = 0; j < m; ++ j )
      {
      v[j] = colPtrs[j]->GetTuple(i)[0];
      //cout << colPtrs[j]->GetName() << ": " << v[j] << " j=" << j << "\n";
      }
    // Update column products. Equation 3.12 from the SAND report.
    x = rv + m;
    for ( cpIt = colPairs.begin(); cpIt != colPairs.end(); ++ cpIt, ++ x )
      {
      // cpIt->first is a pair of indices into colPtrs used to specify (u,v) or (s,t)
      // cpIt->first.first is the index of u or s
      // cpIt->first.second is the index of v or t
      *x += 
        ( v[cpIt->first.first] - rv[cpIt->first.first] ) * // \delta_{u,2,1} = s - \mu_{u,1}
        ( v[cpIt->first.second] - rv[cpIt->first.second] ) * // \delta_{v,2,1} = t - \mu_{v,1}
        i / ( i + 1. ); // \frac{n_1 n_2}{n_1 + n_2} = \frac{n_1}{n_1 + 1}
      }
    // Update running column averages. Equation 1.1 from the SAND report.
    x = rv;
    for ( vtkIdType j = 0; j < m; ++ j, ++ x )
      {
      *x += ( v[j] - *x ) / ( i + 1 );
      }
    }

  outMeta->SetNumberOfBlocks( 1 );
  outMeta->SetBlock( 0, sparseCov );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Raw Sparse Covariance Data" );
  sparseCov->Delete();
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeCholesky( vtksys_stl::vector<double*>& a, vtkIdType m )
{
  // Some macros to make the Cholevsky decomposition algorithm legible:
#ifdef A
#  undef A
#endif
#ifdef L
#  undef L
#endif
#define A(i,j) ( j >= i ? a[j][i] : a[i][j] )
#define L(i,j) a[j][i + 1]

  double tmp;
  for ( vtkIdType i = 0; i < m; ++ i )
    {
    L(i,i) = A(i,i);
    for ( vtkIdType k = 0; k < i; ++ k )
      {
      tmp = L(i,k);
      L(i,i) -= tmp * tmp;
      }
    L(i,i) = sqrt( L(i,i) );
    for ( vtkIdType j = i + 1; j < m; ++ j )
      {
      L(j,i) = A(j,i);
      for ( vtkIdType k = 0; k < i; ++ k )
        {
        L(j,i) -= L(j,k) * L(i,k);
        }
      L(j,i) /= L(i,i);
      }
    }
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Derive( vtkDataObject* outMetaDO )
{
  vtkMultiBlockDataSet* outMeta = vtkMultiBlockDataSet::SafeDownCast( outMetaDO );
  vtkTable* sparseCov;
  vtkStringArray* ocol1;
  vtkStringArray* ocol2;
  vtkDoubleArray* mucov;
  if (
    ! outMeta || outMeta->GetNumberOfBlocks() < 1 ||
    ! ( sparseCov = vtkTable::SafeDownCast( outMeta->GetBlock( 0 ) ) ) ||
    ! ( ocol1 = vtkStringArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_KEYCOLUMN1 ) ) ) ||
    ! ( ocol2 = vtkStringArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_KEYCOLUMN2 ) ) ) ||
    ! ( mucov = vtkDoubleArray::SafeDownCast( sparseCov->GetColumnByName( VTK_MULTICORRELATIVE_ENTRIESCOL ) ) )
    )
    {
    return;
    }

  vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator reqIt;
  vtksys_stl::set<vtkStdString>::const_iterator colIt;
  vtksys_stl::set<vtksys_stl::pair<vtkStdString,vtkDataArray*> > allColumns;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType> colPairs;
  vtksys_stl::map<vtksys_stl::pair<vtkIdType,vtkIdType>,vtkIdType>::iterator cpIt;
  vtksys_stl::map<vtkStdString,vtkIdType> colNameToIdx;
  vtksys_stl::vector<vtkDataArray*> colPtrs;
  // Reconstruct information about the computed sums from the raw data.
  // The first entry is always the sample size
  double n = mucov->GetValue( 0 );
  vtkIdType m = 0;
  vtkIdType i;
  vtkIdType nmucov = mucov->GetNumberOfTuples();
  for ( i = 1; i < nmucov && ocol2->GetValue( i ).empty(); ++ i, ++ m )
    {
    colNameToIdx[ocol1->GetValue( i )] = m;
    }
  for ( ; i < nmucov; ++ i )
    {
    vtksys_stl::pair<vtkIdType,vtkIdType> idxs( colNameToIdx[ocol1->GetValue(i)], colNameToIdx[ocol2->GetValue(i)] );
    colPairs[idxs] = i - 1;
    }
  double* rv = mucov->GetPointer( 0 ) + 1;
  double* x = rv;

  // Create an output table for each request and fill it in using the mucov array (the first table in
  // outMeta and which is presumed to exist upon entry to Derive).
  // Note that these tables are normalized by the number of samples.
  outMeta->SetNumberOfBlocks( 1 + static_cast<int>( this->Internals->Requests.size() ) );
  // For each request:
  i = 0;
  double scale = 1. / ( n - 1 ); // n -1 for unbiased variance estimators
  for ( reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++ reqIt, ++ i )
    {
    vtkStringArray* colNames = vtkStringArray::New();
    colNames->SetName( VTK_MULTICORRELATIVE_COLUMNAMES );
    vtkDoubleArray* colAvgs = vtkDoubleArray::New();
    colAvgs->SetName( VTK_MULTICORRELATIVE_AVERAGECOL );
    vtksys_stl::vector<vtkDoubleArray*> covCols;
    vtksys_stl::vector<double*> covPtrs;
    vtksys_stl::vector<int> covIdxs;
    vtksys_ios::ostringstream reqNameStr;
    reqNameStr << "Cov(";
    // For each column in the request:
    for ( colIt = reqIt->begin(); colIt != reqIt->end(); ++ colIt )
      {
      vtksys_stl::map<vtkStdString,vtkIdType>::iterator idxIt = colNameToIdx.find( *colIt );
      // Ignore invalid column names
      if ( idxIt != colNameToIdx.end() )
        {
        // Create a new column for the covariance matrix output
        covIdxs.push_back( idxIt->second );
        colNames->InsertNextValue( *colIt );
        vtkDoubleArray* arr = vtkDoubleArray::New();
        arr->SetName( colIt->c_str() );
        covCols.push_back( arr );
        if ( colIt == reqIt->begin() )
          {
          reqNameStr << *colIt;
          }
        else
          {
          reqNameStr << "," << *colIt;
          }
        }
      }
    reqNameStr << ")";
    covCols.push_back( colAvgs );
    colNames->InsertNextValue( "Cholesky" ); // Need extra row for lower-triangular Cholesky decomposition

    // We now have the total number of columns in the output.
    // Allocate memory for the correct number of rows and fill in values.
    vtkIdType reqCovSize = colNames->GetNumberOfTuples();
    colAvgs->SetNumberOfTuples( reqCovSize );
    vtkTable* covariance = vtkTable::New();
    outMeta->SetBlock( i + 1, covariance );
    outMeta->GetMetaData( static_cast<unsigned>( i + 1 ) )->Set( vtkCompositeDataSet::NAME(), reqNameStr.str().c_str() );
    covariance->Delete(); // outMeta now owns covariance
    covariance->AddColumn( colNames );
    covariance->AddColumn( colAvgs );
    colNames->Delete();
    colAvgs->Delete();

    vtkIdType j = 0;
    for ( vtksys_stl::vector<vtkDoubleArray*>::iterator arrIt = covCols.begin(); arrIt != covCols.end(); ++ arrIt, ++ j )
      {
      (*arrIt)->SetNumberOfTuples( reqCovSize );
      (*arrIt)->FillComponent( 0, 0. );
      x = (*arrIt)->GetPointer( 0 );
      covPtrs.push_back( x );
      if ( *arrIt != colAvgs )
        { // column is part of covariance matrix
        covariance->AddColumn( *arrIt );
        (*arrIt)->Delete();
        for ( vtkIdType k = 0; k <= j; ++ k, ++ x )
          {
          *x = rv[colPairs[vtksys_stl::pair<vtkIdType,vtkIdType>( covIdxs[k], covIdxs[j] )]] * scale;
          }
        }
      else
        { // column holds averages
        for ( vtkIdType k = 0; k < reqCovSize - 1; ++ k, ++ x )
          {
          *x = rv[covIdxs[k]];
          }
        *x = static_cast<double>( n );
        }
      }
    vtkMultiCorrelativeCholesky( covPtrs, reqCovSize - 1 );
    }
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::Assess( vtkTable* inData, 
                                            vtkDataObject* inMetaDO, 
                                            vtkTable* outData )
{
  vtkMultiBlockDataSet* inMeta = vtkMultiBlockDataSet::SafeDownCast( inMetaDO );
  if ( ! inMeta || ! outData )
    {
    return;
    }

  if ( inData->GetNumberOfColumns() <= 0 )
    {
    return;
    }

  vtkIdType nsamples = inData->GetNumberOfRows();
  if ( nsamples <= 0 )
    {
    return;
    }

  // For each request, add a column to the output data related to the likelihood of each input datum wrt the model in the request.
  // Column names of the metadata and input data are assumed to match (no mapping using AssessNames or AssessParameters is done).
  // The output columns will be named "this->AssessNames->GetValue(0)(A,B,C)" where "A", "B", and "C" are the column names specified in the
  // per-request metadata tables.
  int nb = static_cast<int>( inMeta->GetNumberOfBlocks() );
  AssessFunctor* dfunc = 0;
  for ( int req = 1; req < nb; ++ req )
    {
    vtkTable* reqModel = vtkTable::SafeDownCast( inMeta->GetBlock( req ) );
    if ( ! reqModel )
      { // silently skip invalid entries. Note we leave assessValues column in output data even when it's empty.
      continue;
      }

    this->SelectAssessFunctor( inData, 
                               reqModel, 
                               0, 
                               dfunc );
    vtkMultiCorrelativeAssessFunctor* mcfunc = static_cast<vtkMultiCorrelativeAssessFunctor*>( dfunc );
    if ( ! mcfunc )
      {
      vtkWarningMacro( "Request " 
                       << req - 1 
                       << " could not be accommodated. Skipping." );
      if ( dfunc )
        {
        delete dfunc;
        }
      continue;
      }

    // Create the outData columns
    int nv = this->AssessNames->GetNumberOfValues();
    vtkStdString* names = new vtkStdString[nv];
    for ( int v = 0; v < nv; ++ v )
      {
      vtksys_ios::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue( v )
                    << "(";
      for ( int i = 0; i < mcfunc->GetNumberOfColumns(); ++ i )
        {
        if ( i > 0 )
          {
          assessColName << ",";
          }
        assessColName << mcfunc->GetColumn( i )->GetName();
        }
      assessColName << ")";

      vtkDoubleArray* assessValues = vtkDoubleArray::New();
      names[v] = assessColName.str().c_str(); // Storing names to be able to use SetValueByName which is faster than SetValue
      assessValues->SetName( names[v] );
      assessValues->SetNumberOfTuples( nsamples );
      outData->AddColumn( assessValues );
      assessValues->Delete();
      }

    // Assess each entry of the column
    vtkVariantArray* assessResult = vtkVariantArray::New();
    for ( vtkIdType r = 0; r < nsamples; ++ r )
      {
      (*dfunc)( assessResult, r );
      for ( int v = 0; v < nv; ++ v )
        {
        outData->SetValueByName( r, names[v], assessResult->GetValue( v ) );
        }
      }
    
    assessResult->Delete();

    delete dfunc;
    delete [] names;
    }
}

// ----------------------------------------------------------------------
vtkMultiCorrelativeAssessFunctor* vtkMultiCorrelativeAssessFunctor::New()
{
  return new vtkMultiCorrelativeAssessFunctor;
}

// ----------------------------------------------------------------------
bool vtkMultiCorrelativeAssessFunctor::Initialize( vtkTable* inData, 
                                                   vtkTable* reqModel, 
                                                   bool cholesky )
{
  vtkDoubleArray* avgs = vtkDoubleArray::SafeDownCast( reqModel->GetColumnByName( VTK_MULTICORRELATIVE_AVERAGECOL ) );
  if ( ! avgs )
    {
    vtkGenericWarningMacro( "Multicorrelative request without a \"" VTK_MULTICORRELATIVE_AVERAGECOL "\" column" );
    return false;
    }
  vtkStringArray* name = vtkStringArray::SafeDownCast( reqModel->GetColumnByName( VTK_MULTICORRELATIVE_COLUMNAMES ) );
  if ( ! name )
    {
    vtkGenericWarningMacro( "Multicorrelative request without a \"" VTK_MULTICORRELATIVE_COLUMNAMES "\" column" );
    return false;
    }

  vtksys_stl::vector<vtkDataArray*> cols; // input data columns
  vtksys_stl::vector<double*> chol; // Cholesky matrix columns. Only the lower triangle is significant.
  vtkIdType m = reqModel->GetNumberOfColumns() - 2;
  vtkIdType i;
  for ( i = 0; i < m ; ++ i )
    {
    vtkStdString colname( name->GetValue( i ) );
    vtkDataArray* arr = vtkDataArray::SafeDownCast( inData->GetColumnByName( colname.c_str() ) );
    if ( ! arr )
      {
      vtkGenericWarningMacro( "Multicorrelative input data needs a \"" << colname.c_str() << "\" column" );
      return false;
      }
    cols.push_back( arr );
    vtkDoubleArray* dar = vtkDoubleArray::SafeDownCast( reqModel->GetColumnByName( colname.c_str() ) );
    if ( ! dar )
      {
      vtkGenericWarningMacro( "Multicorrelative request needs a \"" << colname.c_str() << "\" column" );
      return false;
      }
    chol.push_back( dar->GetPointer( 1 ) );
    }

  // OK, if we made it this far, we will succeed
  this->Columns = cols;
  this->Center = avgs->GetPointer( 0 );
  this->Tuple.resize( m );
  this->EmptyTuple = vtksys_stl::vector<double>( m, 0. );
  if ( cholesky )
    {
    vtkMultiCorrelativeInvertCholesky( chol, this->Factor ); // store the inverse of chol in this->Factor, F
    vtkMultiCorrelativeTransposeTriangular( this->Factor, m ); // transposing F makes it easier to use in the () operator.
    }
#if 0
  // Compute the normalization factor to turn X * F * F' * X' into a cumulance.
  if ( m % 2 == 0 )
    {
    this->Normalization = 1.0;
    for ( i = m / 2 - 1; i > 1; -- i )
      {
      this->Normalization *= i;
      }
    }
  else
    {
    this->Normalization = sqrt( 3.141592653589793 ) / ( 1 << ( m / 2 ) );
    for ( i = m - 2; i > 1; i -= 2 )
      {
      this->Normalization *= i;
      }
    }
#endif // 0

  return true;
}

// ----------------------------------------------------------------------
void vtkMultiCorrelativeStatistics::SelectAssessFunctor( vtkTable* inData, 
                                                         vtkDataObject* inMetaDO,
                                                         vtkStringArray* vtkNotUsed(rowNames), 
                                                         AssessFunctor*& dfunc )
{
  (void)inData;

  dfunc = 0;
  vtkTable* reqModel = vtkTable::SafeDownCast( inMetaDO );
  if ( ! reqModel )
    {
    return;
    }

  vtkMultiCorrelativeAssessFunctor* mcfunc = vtkMultiCorrelativeAssessFunctor::New();
  if ( ! mcfunc->Initialize( inData, reqModel ) )
    {
    delete mcfunc;
    }
  dfunc = mcfunc;
}

