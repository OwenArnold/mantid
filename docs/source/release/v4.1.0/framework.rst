=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

Improvements
############

- :ref:`LoadEventNexus <algm-LoadEventNexus>` has an additional option `LoadNexusInstrumentXML` = `{Default, True}`,  which controls whether or not the embedded instrument definition is read from the NeXus file.
- The numerical integration absorption algorithms (:ref:`AbsorptionCorrection <algm-AbsorptionCorrection>`, :ref:`CuboidGaugeVolumeAbsorption <algm-CuboidGaugeVolumeAbsorption>`, :ref:`CylinderAbsorption <algm-CylinderAbsorption>`, :ref:`FlatPlateAbsorption <algm-FlatPlateAbsorption>`) have been modified to use a more numerically stable method for performing the integration, `pairwise summation <https://en.wikipedia.org/wiki/Pairwise_summation>`_.
- Improved support for thin-walled hollow cylinder shapes in :ref:`algm-MonteCarloAbsorption`
- Support has been added for negative indexing of :ref:`WorkspaceGroups <WorkspaceGroup>`.
  Try :code:`ws_group[-1]` to get the last workspace in the WorkspaceGroup :code:`ws_group`.
- :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>` has a new property `ScatterFrom` which allows for calculating the correction for the other components (i.e. container and environment). Also, a bug was fixed where the amount of the sample being scattered through was being under-reported for hollow objects.

Removed
#######

- The deprecated version 1 of the `FindEPP` algorithm has been removed. Use :ref:`FindEPP-v2 <algm-FindEPP>` instead.

Data Objects
------------

Python
------

:ref:`Release 4.1.0 <v4.1.0>`
