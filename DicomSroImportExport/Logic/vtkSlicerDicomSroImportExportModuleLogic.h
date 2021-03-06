/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerDicomSroImportExportModuleLogic - slicer logic class for SRO importing
// .SECTION Description
// This class manages the logic associated with reading Dicom spatial registration object


#ifndef __vtkSlicerDicomSroImportExportModuleLogic_h
#define __vtkSlicerDicomSroImportExportModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include <vector>

#include "vtkSlicerDicomSroImportExportModuleLogicExport.h"

class vtkCollection;
class vtkMatrix4x4;
class vtkMRMLGridTransformNode;
class vtkMRMLLinearTransformNode;
class vtkSlicerDICOMLoadable;
class vtkSlicerDicomSroReader;
class vtkStringArray;

/// \ingroup SlicerRt_DicomSroImportLogicExport
class VTK_SLICER_DICOMSROIMPORTEXPORT_MODULE_LOGIC_EXPORT vtkSlicerDicomSroImportExportModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDicomSroImportExportModuleLogic *New();
  vtkTypeMacro(vtkSlicerDicomSroImportExportModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Examine a list of file lists and determine what objects can be loaded from them
  void ExamineForLoad(vtkStringArray* fileList, vtkCollection* loadables);

  /// Load DICOM SRO series from file name
  /// \return True if loading successful
  bool LoadDicomSro(vtkSlicerDICOMLoadable* loadable);

protected:
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

  /// Load DICOM spatial registration objects into the MRML scene
  /// \return Loaded transform node
  vtkMRMLLinearTransformNode* LoadSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkSlicerDICOMLoadable* loadable);

  /// Load DICOM deformable spatial registration objects into the MRML scene
  /// \return Loaded transform node
  vtkMRMLGridTransformNode* LoadDeformableSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkSlicerDICOMLoadable* loadable);

  /// Load DICOM spatial fiducial objects into the MRML scene
  /// NOTE: Not yet implemented
  /// \return Success flag
  bool LoadSpatialFiducials(vtkSlicerDicomSroReader* regReader, vtkSlicerDICOMLoadable* loadable);

protected:
  vtkSlicerDicomSroImportExportModuleLogic();
  ~vtkSlicerDicomSroImportExportModuleLogic() override;
  vtkSlicerDicomSroImportExportModuleLogic(const vtkSlicerDicomSroImportExportModuleLogic&) = delete;
  void operator=(const vtkSlicerDicomSroImportExportModuleLogic&) = delete;
};

#endif
