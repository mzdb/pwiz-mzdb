#ifndef DE_BDAL_CPP_IO_BAF2SQL_SCHEMA_H
#define DE_BDAL_CPP_IO_BAF2SQL_SCHEMA_H

// developers: make sure that every time you change this schema you
// increase its version number in this file, the version number of the
// baf2sql artifact, as well as the documentation in this file.
#define BAF2SQL_SCHEMA_VERSION "1.3.8"

namespace {

/** \file schema.h Definition and documentation of the SQLite cache
    database schema.
    
\mainpage Documentation of the SQLite cache database schema
    
\section baf2sql Philosophy behind BAF2SQL
    
BAF2SQL enables read-only access to the data in a BAF file through two
layers: an SQLite database "cache", and a binary storage interface.
    
<ul>

<li>Metadata describing the spectra and the acquisition are available
through an SQLite database. The database schema is documented
here. This SQLite database will be generated automatically for each
BAF; see the documentation of the language binding (C++, C, C#) you
are going to use.

<li>The actual spectrum data, i.e. any stored arrays of binary data,
only have a 64-bit integer ID in the SQL database. Using this ID,
you can retrieve the actual data using the binary storage interface
using a function such as readArrayData(); see the documentation of
the language binding (C++, C, C#) you are going to use.

</ul>

Auto-generated HTML documentation for the SQL database schema is not
available unfortunately; for an explanation of the tables and their
fields please look at the source code of this file (schema.h).

\section notes Notes on the array IDs

The nine IDs (starting with ProfileMzId) in the Spectra table are
integers using which you can retrieve the actual array data from the
binary storage interface functions.

Note: array IDs are 64-bit unsigned integers in general - when using
the C++ interface 'CppSQLite3' for SQLite, this requires you to use
boost::lexical_cast<uint64_t> on the result of
CppSQLite3Query::fieldValue() to extract this number from a row.

Some of the ID fields in a row may not be set (i.e., NULL), indicating
that there is no corresponding array in the BAF file. However, even if
the field is set to some integer number, when you go to the binary
storage and ask for the corresponding array, it might turn out to have
length zero, even though other arrays corresponding to the same
spectrum may have non-empty arrays (i.e., LineIndexId may be
non-empty, but LineSnrId might be empty). This is normal behaviour and
simply means that during acquisition no SNR was recorded. For
performance reasons we decided to not clear the corresponding ID field
in the row from the beginning (this would require scanning the entire
BAF file).

Never try to interpret the ID. It is an opaque handle, and you should
only use it to go to the read_array() functions.

Don't expect that the ID will remain constant forever. If you open the
same BAF the next day, the ID might have changed (especially when this
schema changes). Just take the ID from the fresh SQLite cache, then
get the array with it; don't store the ID in your application.

\section general General notes

There may be more tables in the actual database than documented
here. This should not disturb you.

A table may have more columns than documented below (e.g., for
internal prototyping). This should not disturb you. Be careful when
using "SELECT *" statements; don't expect specific columns at specific
indices in that case.

You should not try to modify the database yourself. It is read-only
for you.

******************************************************************** */


const char make_properties_table[] = 

    // Key-value pairs containing global information about this
    // analysis.
    "CREATE TABLE Properties ("

    // The following keys are supported:
    // 
    //   AcquisitionSoftware - e.g., "flexImaging", "micrOTOFcontrol"
    //
    //   AcquisitionSoftwareVendor - e.g., "Bruker"
    //
    //   AcquisitionSoftwareVersion - e.g., "5.0"
    //
    //   InstrumentVendor - e.g., "Bruker"
    //
    //   InstrumentFamily - a number, e.g., 7 for "maXis impact". A
    //     list of mappings from this number to human-readable strings
    //     and other instrument information can be provided
    //     separately.
    //
    "Key TEXT PRIMARY KEY,"

    "Value TEXT"

    ");";

const char make_spectra_table[] = 

    // The 'Spectra' table contains one row per spectrum. A spectrum
    // may have line-spectrum or profile-spectrum data associated with
    // it, or both.
    "CREATE TABLE Spectra ("

    // the number of this spectrum
    "Id integer PRIMARY KEY,"

    // retention time (in seconds)
    "Rt double,"

    // segment number
    "Segment integer,"

    // (link to AcquisitionKeys.Id)
    "AcquisitionKey integer,"

    // (link to Spectra.Id, may be NULL); for MS^n spectra, specifies
    // the "parent spectrum" from which this spectrum was derived
    // using the steps recorded in the 'Steps' table.
    "Parent integer,"

    // mass range of acquisition: m/z start
    "MzAcqRangeLower double,"

    // mass range of acquisition: m/z stop
    "MzAcqRangeUpper double,"

    // sum / integral of intensity over the entire mass range of this
    // spectrum
    "SumIntensity double,"

    // maximum intensity occurring in the mass range of this spectrum
    "MaxIntensity double,"

    // "array index" <-> "m/z" transformator (-> Transformators.Id)
    "TransformatorId integer,"

    // profile-spectrum m/z axis (ID for binary storage interface)
    "ProfileMzId integer,"

    // profile-spectrum intensities (ID for binary storage interface)
    "ProfileIntensityId integer,"

    // line-spectrum indices (ID for binary storage interface)
    "LineIndexId integer,"

    // line-spectrum m/z values (ID for binary storage interface)
    "LineMzId integer,"

    // line-spectrum peak intensities (ID for binary storage
    // interface)
    "LineIntensityId integer,"

    // line-spectrum peak widths (ID for binary storage interface)
    "LinePeakWidthId integer,"

    // line-spectrum peak areas (ID for binary storage interface)
    "LinePeakAreaId integer,"

    // line-spectrum peak SNRs (ID for binary storage interface)
    "LineSnrId integer,"

    // line-spectrum goodness values (ID for binary storage interface)
    "LineGoodnessId integer"

    ");";

const char make_steps_table[] = 

    // The 'Steps' table contains information about how MS^n spectra
    // etc. are produced: a kind of history of the isolation and
    // fragmentation steps that have led to a "target spectrum" during
    // acquisition.
    //
    // Example: given a spectrum ID 'X' from the 'Spectra' table, you
    // can find out which steps have led to that spectrum using:
    // 
    //   SELECT * FROM Steps WHERE TargetSpectrum=X ORDER BY Number
    // 
    // The corresponding parent spectrum, if there is one, is in the
    // Spectra table entry for the target spectrum:
    //
    //   SELECT Parent FROM Spectra WHERE Id=X
    //
    "CREATE TABLE Steps ("
    
    // (link to Spectra.Id); this step belongs to the group of
    // steps that have led to spectrum # TargetSpectrum
    "TargetSpectrum integer,"
            
    // Consecutive numbering of the steps inside this group of
    // steps:
    "Number integer,"

    // One of the values of enum IsolationMode defined in
    // bdalenumerations.h; currently possible values:
    //
    // ISOL_MODE_ON  = 0x00000001   isolation is switched on
    //
    "IsolationType integer,"

    // One of the values of enum FragmentationMode defined in
    // bdalenumerations.h; currently possible values:
    //
    // FRAG_MODE_CID = 0x00000001   (low-energy) collision-induced dissociation
    //
    "ReactionType integer,"

    // 0 = MS (no fragmentation), 1 = MS/MS, ...
    "MsLevel integer,"

    // Parent mass (trigger mass, not necessarily monoisotopic)
    "Mass double"

    ");";

const char make_transformators_table[] = 

    "CREATE TABLE Transformators ("

    "Id integer PRIMARY KEY,"

    // Opaque blob object from calibration-core.
    "Blob blob"

    ");";

const char make_reference_transformators_table[] =
    
    "CREATE TABLE ReferenceTransformators ("

    "Id integer PRIMARY KEY,"

    // (link to AcquisitionKeys.Id)
    "AcquisitionKey integer,"

    // "array index" <-> "m/z" transformator (-> Transformators.Id)
    "TransformatorId integer"

    ");";


const char make_acqkey_table[] =
    
    "CREATE TABLE AcquisitionKeys ("

    "Id integer PRIMARY KEY,"

    // ???
    "Polarity integer,"

    // ???
    "ScanMode integer,"

    // ???
    "AcquisitionMode integer,"

    // 0 = MS (no fragmentation), 1 = MS/MS, ...
    "MsLevel integer"

    ");";

// =========================================================================================================== //
// =========================================================================================================== //
// =========================================================================================================== //

const char make_per_spec_var_settings[] = 

    // Internal table; a user should refer to the "Variables" view
    // instead. This table stores variable settings that apply to a
    // single spectrum only.
    "CREATE TABLE PerSpectrumVariables ("

    "Spectrum INTEGER,"

    "Variable INTEGER,"

    "Value /* affinity NONE */,"

    // (Speeds up Variables view.)
    "PRIMARY KEY (Spectrum, Variable)"

    ");";
    // Note: WITHOUT ROWID optimization useful here, but requires
    // SQLite 3.8.2 or later.


const char make_per_seg_acqkey_var_settings[] = 

    // Internal table; a user should refer to the "Variables" view
    // instead. This table stores variable settings that apply to all
    // spectra that are in a specific segment and that have a specific
    // acquisition key.
    "CREATE TABLE PerSegAcqKeyVariables ("

    "Segment INTEGER,"

    "AcquisitionKey INTEGER,"

    "Variable INTEGER,"

    "Value /* affinity NONE */,"

    "PRIMARY KEY (Segment, AcquisitionKey, Variable)"

    ");";
    // Note: WITHOUT ROWID optimization useful here, but requires
    // SQLite 3.8.2 or later.


const char make_per_segment_var_settings[] = 

    // Internal table; a user should refer to the "Variables" view
    // instead. This table stores variable settings that apply to all
    // spectra that are in a specific segment, independent of their
    // acquisition key.
    //
    // This is currently the "coarsest" way of setting variables in a
    // BAF. In principle, there are "per-analysis" settings, but none
    // of our acquisition software uses these, so they are not taken
    // into account in this schema.
    //
    "CREATE TABLE PerSegmentVariables ("

    "Segment INTEGER,"

    "Variable INTEGER,"

    "Value /* affinity NONE */,"

    "PRIMARY KEY (Segment, Variable)"

    ");";
    // Note: WITHOUT ROWID optimization useful here, but requires
    // SQLite 3.8.2 or later.



/// Define some helper SQL views and the main view 'Variables' (see
/// below).
const char make_variable_views[] =

    // Internal view that makes the per-segment-and-acquisition-key
    // variables accessible through spectrum number.
    "CREATE VIEW PerSegAcqKeyVariables_ForSpectrum AS "
    "SELECT Spectra.Id AS Spectrum, Variable, Value FROM PerSegAcqKeyVariables "
    "JOIN Spectra ON PerSegAcqKeyVariables.Segment        = Spectra.Segment "
                "AND PerSegAcqKeyVariables.AcquisitionKey = Spectra.AcquisitionKey;"
    
    // Internal view that makes the per-segment variables accessible
    // through spectrum number.
    "CREATE VIEW PerSegmentVariables_ForSpectrum AS "
    "SELECT Spectra.Id AS Spectrum, Variable, Value FROM PerSegmentVariables "
    "JOIN Spectra ON PerSegmentVariables.Segment = Spectra.Segment;"

    // This table collects the IDs of all variables that have been
    // scanned for in the Baf to which this SQLite cache
    // corresponds. Currently this is precisely the set of IDs listed
    // in variables.h. (In the future, it may only be a subset
    // requested by the user of Baf2Sql.)
    "CREATE TABLE SupportedVariables (Variable INTEGER PRIMARY KEY);"


    //
    // This is the top-level view on the variables which most users
    // should use.
    //
    // Example:
    //
    //   SELECT Spectrum, Variable, Value FROM Variables
    //      WHERE Variable IN (1,2,3) AND Spectrum IN (25145,1,23);
    //
    // Possible output:
    //
    //   1|1|
    //   1|2|872
    //   1|3|
    //   23|1|273647
    //   23|2|1744
    //   23|3|2
    //   25145|1|273647
    //   25145|2|1744
    //   25145|3|2
    //
    // Non-set variables have NULL Value, as in the first and third
    // result rows above.
    //
    // Don't rely on any implicit ordering of the result. Add an ORDER
    // BY clause to the SQL statement if required.
    //
    // -------------------------------------------------------------------------------
    //
    // Another example: a "variable chromatogram", i.e., the
    // dependence of a variable's value on time. Here, we look at the
    // temperature #1, which has variable number 9 (see
    // variables.h). In addition, we select only unfragmented
    // (MsLevel=0) spectra using a JOIN with the AcquisitionKeys
    // table:
    //
    //   SELECT s.Rt, v.Value FROM Variables v
    //                        JOIN Spectra s ON s.Id=v.Spectrum
    //                        JOIN AcquisitionKeys ak ON s.AcquisitionKey=ak.Id
    //      WHERE v.Variable=9 AND ak.MsLevel=0
    //      ORDER BY s.Rt LIMIT 10;
    //
    // Possible output:
    //
    //   0.337|33.2898963386871
    //   1.118|33.2898373753002
    //   1.337|33.2898373753002
    //   1.556|33.2898373753002
    //   1.775|33.2898373753002
    //   1.995|33.2898373753002
    //   2.321|33.2898373753002
    //   2.649|33.2898373753002
    //   2.868|33.2898373753002
    //   3.196|33.2898790015472
    //
    // -------------------------------------------------------------------------------
    //
    // Note: in principle, the BAF format supports vector-valued
    // variables, but none of these have been requested for inclusion
    // in Baf2Sql, so this schema doesn't take those into account yet.
    //
    "CREATE VIEW Variables AS "
        "SELECT a.Spectrum Spectrum, a.Variable Variable, COALESCE(t1.Value, t2.Value, t3.Value) Value "
            "FROM ( SELECT Spectra.Id as Spectrum, Variable FROM Spectra, SupportedVariables ) a "
            "LEFT JOIN PerSpectrumVariables t1              ON t1.Spectrum=a.Spectrum AND t1.Variable=a.Variable "
            "LEFT JOIN PerSegAcqKeyVariables_ForSpectrum t2 ON t2.Spectrum=a.Spectrum AND t2.Variable=a.Variable "
            "LEFT JOIN PerSegmentVariables_ForSpectrum t3   ON t3.Spectrum=a.Spectrum AND t3.Variable=a.Variable;";
    
} // anonymous namespace

#endif // DE_BDAL_CPP_IO_BAF2SQL_SCHEMA_H
