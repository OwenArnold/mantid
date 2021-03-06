.. _RemoteJobSubmissionAPI:

=========================
Remote Job Submission API
=========================

.. contents::
  :local:

This document describes the web service API that the Mantid Framework uses to submit algorithms to remote compute
resources. The API is designed to be flexible enough that it can be implemented for just about any compute resource at
nearly any facility. This document should hopefully contain enough detail for programmers & sysadmins at other
facilities to implement the API and allow Mantid users to submit jobs to compute resources at those other facilities.

A 'compute resource' may be any computer capable of running the Mantid framework. This could range from a single large
server, to some sort of Beowulf cluster to machines with specialized hardware such as GPU's or Intel MIC's. The idea is
that the 'compute resource' will be used to execute tasks that are impractical to run on the user's local workstation.

The reference implementation is the Fermi cluster at the Oak Ridge Spallation Neutron Source (fermi.ornl.gov). Specific
details of the implementation on Fermi are used as examples throughout this document.

Basic Requirements For API Implementations
==========================================

The Mantid framework has certain expectations for the API and it's the responsibility of API implementations to meet
these expectations:

#. The API provides a mechanism for executing python scripts in an environment that includes the Mantid framework. (ie:
   one where "from mantid.simpleapi import \*" works) (Note: exactly \*how\* this happens is purposely left up to the
   implementor.)
#. Scripts are executed in a batch environment - interactive scripts are not supported and scripts must be able to run
   without input from the user.
#. No mechanism for passing command line parameters to the python scripts is provided. Things like experiment and run
   numbers must be hard-coded into the script. (We consider this acceptable because we expect a new script to be
   auto-generated by MantidPlot for each job.)
#. Scripts can write output to what is the current working directory when the script starts. These output files will be
   available for download back to MantidPlot (or wherever) once the script finishes.
#. Files that were uploaded prior to running the script will be available from the script's working directory.
#. Authentication is used to prevent individual users from interfering with others' jobs (or even reading their files)
#. Data from the various API calls is returned in two ways: the standard HTTP status codes (200, 404, 500, etc...) and
   JSON text in the body. (Note: There's no HTML markup in the output.) In the case of error status codes, the JSON
   output will contain a field called 'error_msg' who's value is a string describing the error.
#. Clients should assume that all parts of the URL - including query parameters - are case sensitive. However, servers
   don't \*have\* to accept URL's with improper case, but may if it makes the code simpler.

Versioning
==========

A quick Google search shows that versioning web API's is tricky. The technique employed here will be to use a major
version plus optional extensions.

Between major versions there are no guarantees of backwards or forwards compatibility. Changing the major version is
essentially starting over from scratch with a clean slate. The major version starts at 1 and will hopefully never
change.

Backwards-compatible changes are handled by means of optional extensions.

A server must implement all of the functions defined in the base level of the version and a client may assume those
functions exist. A server is NOT required to implement the extensions (they're optional) and a client may query the
server to discover which extensions are implemented. (Note that what the server returns is really just a name. It's up
to the client and server implementers to agree on exactly what that name means and then document it - presumably here in
this API doc.)

Backwards-incompatible changes to the API are not allowed. That includes the URL itself, the GET or POST variables it
expects and the JSON that it outputs. If changes are needed, they can be handled through the extension mechanism. For
example, more GET or POST variables could be accepted by the server, so long as they are not required. An extension
should be created (and documented here) so that a client may query the server about whether it supports the new
variables.

Old GET or POST variables cannot be deleted however. This would break clients that expect to use them. If there's a case
where old varibles no longer make sense, then a completely new URL should be created (and again, documented as an
extension).

Similar rules apply to the JSON data returned by the API. Extra fields can be added to the JSON returned by a URL, but
original fields may not be removed.

It's worth noting that it is possible for extensions to be mutually exclusive.

Authentication
==============

The initial API uses HTTP Basic-Auth name/password combo for authentication. (Other methods may be added via API
extensions as described above.) Upon successful authentication, a session cookie is created. All other URL's require
this session cookie.

Because the Basic-Auth scheme does not encrypt the password when it is sent to the server, the use of the HTTPS protocol
is **STRONGLY** encouraged.

Note that HTTP Basic-Auth is simply the mechanism for passing a username/password combo to the web service. Exactly how
the web service uses that combo to authenticate (and authorize) a user is not specified by the API. Individual
implementations are free to do what they like. (For example, the implementation on Fermi checks the username & password
against an LDAP server.)

Transactions
============

The API details below mention starting & stopping transactions. It should be noted that in this context, the word
transaction doesn't really have anything to do with databases. In this context, transactions are a mechanism for the web
service to associate files with specific jobs. Multiple jobs may be submitted under a given transaction.

Note that the API doesn't specify how this association occurs. That is a detail left up to the individual
implementation. However, remember the points in the Basic Requirements section above about scripts reading from and
writing to their current working directory while not allowing other users to see or modify their files. That implies
that each job will store files in its own directory and will execute scripts from that directory. (This is, in fact, how
the implementation on Fermi works.)

A user must start a transaction after authenticating, but before transferring files or submitting job scripts. When the
user's job (or jobs) has finished and the user no longer needs the files associated with the transaction, he or she
should end the transaction. This will allow the web service to delete the files and recover the disk space.

A Note About File Uploads
=========================

It is generally assumed that the input files for the submitted python scripts are already available on the compute
resource (presumably via some kind of network filesystem). Thus, although the API allows for file uploads, this is
really intended for relatively small support files that a particular script might need. The HTTP protocol really isn't
intended or suitable for transferring the sort of multi-gigabyte files that are likely to be the inputs for these python
scripts.

API v1 URL's
============

General notes:

-  All URL's expect GET requests unless otherwise noted.
-  The session cookie returned by the authentication URL is required by all other URL's (except for the info URL)
-  Success is indicated by an HTTP status code in the 200 range. (Typically, 200, but in some cases 201.) Errors are
   indicated with error codes in the 400 and 500 range.
-  In the case of errors, the JSON output will include a field named "Err_Msg" whose value is a text string describing
   the particular error.


Information
-----------

+--------------------------------+-------------------------------------------------------------------------------------+
| Description                    | Returns information about the server including the API version and supported        |
|                                | extensions.                                                                         |
+--------------------------------+-------------------------------------------------------------------------------------+
| URL                            | <base_url>/info                                                                     |
+--------------------------------+-------------------------------------------------------------------------------------+
| Query Parameters               | None                                                                                |
+--------------------------------+-------------------------------------------------------------------------------------+
| JSON Output                    | API_Version : <integer> API_Extensions : [<extension_1>, <extensions_2>, .... ]     |
|                                | Implementation_Specific_Post_Variables : [ <variable_1>, <variable_2>, .... ]       |
+--------------------------------+-------------------------------------------------------------------------------------+
| Notes                          | May be called without first authenticating. The                                     |
|                                | 'Implementation_Specific_Submit_Variables' field lists the particular POST          |
|                                | variables that this implementation requires when submitting a job. See the          |
|                                | 'Job Submission <#Job_Submission>`__ URL.                                           |
+--------------------------------+-------------------------------------------------------------------------------------+

Authentication
--------------

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Authenticate to the web service.                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/authenticate                                   |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | Username and password are passed in using HTTP Basic      |
|                                                           | Authentication Returns a session cookie which must be     |
|                                                           | passed to the other URL's                                 |
+-----------------------------------------------------------+-----------------------------------------------------------+

Transactions
------------

This URL has two forms: one to start a new transaction and the other to end an existing transaction.

+-------------------------------------------------+--------------------------------------------------------------------+
| Description                                     | Start a new transaction                                            |
+-------------------------------------------------+--------------------------------------------------------------------+
| URL                                             | <base_url>/transaction                                             |
+-------------------------------------------------+--------------------------------------------------------------------+
| Query Parameters                                | Action=Start                                                       |
+-------------------------------------------------+--------------------------------------------------------------------+
| JSON Output                                     | TransID : <string>                                                 |
+-------------------------------------------------+--------------------------------------------------------------------+
| Notes                                           |                                                                    |
+-------------------------------------------------+--------------------------------------------------------------------+

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | End an existing transaction                               |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/transaction                                    |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | Action=Stop TransID=<transaction_id>                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | Once a transaction is stopped, any files associated with  |
|                                                           | it will no longer be available for download and the       |
|                                                           | server is free to delete those files.                     |
+-----------------------------------------------------------+-----------------------------------------------------------+

File Transfer
-------------

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Transfer a file from the server back to the client        |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/download                                       |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | TransID=<transaction ID> File=<filename>                  |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               |                                                           |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | <filename> does not include any path information. The     |
|                                                           | actual directory where the file is stored is chosen by    |
|                                                           | the web service and hidden from the user                  |
+-----------------------------------------------------------+-----------------------------------------------------------+

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Transfer one or more files from the client up to the      |
|                                                           | server                                                    |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/upload                                         |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | This is a POST method Multiple files may be submitted     |
|                                                           | with one call                                             |
|                                                           | File(s) are submitted as                                  |
|                                                           | multipart form data (ie: "Content-Type:                   |
|                                                           | multipart/form-data" header)                              |
|                                                           | File names should not include                             |
|                                                           | any directory or path information. (Exactly where the     |
|                                                           | file is stored is left to the web service to determine.)  |
|                                                           | The transaction ID must also be                           |
|                                                           | specified as form data with a field name of "TransID"     |
|                                                           | On success, returns a "201 - Created" status code         |
+-----------------------------------------------------------+-----------------------------------------------------------+

File Listing
------------

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Return a listing of files associated with the specified   |
|                                                           | transaction                                               |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/files                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | TransID=<transaction ID>                                  |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | Files : [ <file_1>, <file_2>, ... <file_n> ]              |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | No guarantees are made about the order files are listed   |
+-----------------------------------------------------------+-----------------------------------------------------------+

Job Submission
--------------

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Submit a python script for execution on the compute       |
|                                                           | resource                                                  |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/submit                                         |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Mandatory POST Variables                                  | TransID : <trans_id>                                      |
|                                                           | ScriptName : <name_of_python_script>                      |
|                                                           | <name_of_python_script> : <python code>                   |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Optional POST Variables                                   | JobName : <name>                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Implementation Specific POST Variables                    | NumNodes : <number_of_nodes>                              |
|                                                           | CoresPerNode: <cores_per_node>                            |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | JobID : <job_id>                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | This is a POST method                                     |
|                                                           | Request is submitted as multipart form data (ie:          |
|                                                           | "Content-Type: multipart/form-data" header)               |
|                                                           | POST variables listed above are individual form data      |
|                                                           | fields                                                    |
|                                                           | The content of the "ScriptName" field specifies the name  |
|                                                           | of the python script. There must be another field with    |
|                                                           | this name that actually contains the python code. This    |
|                                                           | allows the web service to keep track of multiple scripts  |
|                                                           | associated with the same transaction.                     |
|                                                           | The JobName variable allows the user to specify a name    |
|                                                           | for a job. The name is included in the output of queries. |
|                                                           | (Presumably, the user will pick a name that's more        |
|                                                           | descriptive and easier to remember than the automatically |
|                                                           | assigned job ID.)                                         |
|                                                           | The Implementation Specific Post Variables are - like the |
|                                                           | name says - specific to a particular implementation. They |
|                                                           | may not be applicable to all implementations and it's     |
|                                                           | valid for an implementation to ignore those that aren't.  |
|                                                           | Which variables are required by a specific implementation |
|                                                           | are listed in the `Information <#Information>`__ URL.     |
|                                                           | (The two specified above are used by the Fermi            |
|                                                           | implementation, and would probably be valid for all       |
|                                                           | compute clusters.)                                        |
+-----------------------------------------------------------+-----------------------------------------------------------+

Job Query
---------

This URL has two forms: one to query a specific job and one to query all of a user's jobs.

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Query all jobs submitted by the user                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/query                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | <job_id> : <job_description_object>                       |
|                                                           | <job_id> : <job_description_object>                       |
|                                                           | <job_id> : <job_description_object>                       |
|                                                           | etc...                                                    |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | See below for a description of the job_description_object |
|                                                           | The length of time the compute resource will 'remember'   |
|                                                           | jobs is up to the implementer, but several days should be |
|                                                           | considered an absolute minimum. (A user should be able to |
|                                                           | submit a job on Friday and still be able to query it on   |
|                                                           | Monday morning.)                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Query one specific job submitted by the user              |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/query                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | JobID : <job_id>                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | <job_id> : <job_description_object>                       |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | See below for a description of the job_description_object |
|                                                           | The length of time the compute resource will 'remember'   |
|                                                           | jobs is up to the implementer, but several days should be |
|                                                           | considered an absolute minimum. (A user should be able to |
|                                                           | submit a job on Friday and still be able to query it on   |
|                                                           | Monday morning.)                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+

The job description object is a JSON object who's fields contain specific information about the job. The fields are:

-  TransID - The transaction ID the job is associated with
-  JobName - The name that was given to the submit API
-  ScriptName - The name of the python script that was executed
-  JobStatus - The execution status of the job. Will be one of: RUNNING, QUEUED, COMPLETED, REMOVED, DEFERRED, IDLE or
   UNKNOWN

Job Abort
---------

+-----------------------------------------------------------+-----------------------------------------------------------+
| Description                                               | Abort a previously submitted job. Jobs that are queued    |
|                                                           | will be dequeued. Jobs that are running will be stopped   |
|                                                           | immediately. Jobs that have already completed will simply |
|                                                           | be ignored.                                               |
+-----------------------------------------------------------+-----------------------------------------------------------+
| URL                                                       | <base_url>/abort                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Query Parameters                                          | JobID : <job_id>                                          |
+-----------------------------------------------------------+-----------------------------------------------------------+
| JSON Output                                               | None                                                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Notes                                                     | Returns a 400 error code if the job ID does not exist.    |
+-----------------------------------------------------------+-----------------------------------------------------------+

API v1 Extensions
=================

JOB_DATES
---------

The JOB_DATES extension adds three fields to the job_description_object that is returned by queries. The fields are
"SubmitDate", "StartDate" & "CompletionDate" which represent the dates (including time) that the job was first
submitted, when it started executing and when it stopped (either because it finished or because it was interrupted for
some reason). The values are ISO8601 strings suitable for importing into a Mantid DateAndTime object.

AUTH_USER_NAME
--------------

The AUTH_USER_NAME extension adds a single field the the JSON text returned by the 'info' URL. The field name is
'Authenticated_As' and its value is either the name of the user that's been authenticated, or empty if no authentication
has taken place yet.
