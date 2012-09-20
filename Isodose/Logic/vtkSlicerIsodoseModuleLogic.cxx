/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, RMP, PMH
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLProceduralColorNode.h>
#include "vtkMRMLColorLogic.h"

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageMarchingCubes.h>
#include <vtkImageChangeInformation.h>
#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>
#include <vtkLookupTable.h>
#include <vtkColorTransferFunction.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerIsodoseModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic::vtkSlicerIsodoseModuleLogic()
{
  this->IsodoseNode = NULL;
  this->colorTableID = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic::~vtkSlicerIsodoseModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->IsodoseNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetAndObserveIsodoseNode(vtkMRMLIsodoseNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->IsodoseNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLIsodoseNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLIsodoseNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLIsodoseNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLIsodoseNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
  if (node)
  {
    paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->IsodoseNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
bool vtkSlicerIsodoseModuleLogic::DoseVolumeContainsDose()
{
  if (!this->GetMRMLScene() || !this->IsodoseNode)
  {
    return false;
  }

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->IsodoseNode->GetDoseVolumeNodeId()));

  const char* doseUnitName = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());

  if (doseUnitName != NULL)
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
const char *vtkSlicerIsodoseModuleLogic::GetDefaultLabelMapColorNodeID()
{

  if (this->colorTableID == NULL)
  {
    this->AddDefaultIsodoseColorNode();
  }

  char* temp = "vtkMRMLColorTableNodeUserDefined";
  this->colorTableID = temp;
  return "vtkMRMLColorTableNodeUserDefined";
}

//----------------------------------------------------------------------------
void vtkSlicerIsodoseModuleLogic::AddDefaultIsodoseColorNode()
{
  // create the default color nodes, they don't get saved with the scenes as
  // they'll be created on start up, and when a new
  // scene is opened
  if (this->GetMRMLScene() == NULL)
  {
    vtkWarningMacro("vtkMRMLColorLogic::AddDefaultColorNodes: no scene to which to add nodes\n");
    return;
  }
  
  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);

  // add a random procedural node that covers full integer range
  vtkMRMLColorTableNode* isodoseColorNode = this->CreateIsodoseColorNode();
  this->colorTableID = isodoseColorNode->GetID();

  // Add the node after it has been initialized
  this->GetMRMLScene()->AddNode(isodoseColorNode);

  isodoseColorNode->Delete();

  vtkDebugMacro("Done adding default color nodes");
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
}


//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkSlicerIsodoseModuleLogic::CreateIsodoseColorNode()
{
  vtkDebugMacro("vtkSlicerIsodoseModuleLogic::CreateIsodoseColorNode: making a default mrml colortable node");
  vtkMRMLColorTableNode *colorTableNode = vtkMRMLColorTableNode::New();
  this->colorTableID = colorTableNode->GetID();
  colorTableNode->SetName("IsodoseColor");
  colorTableNode->SetTypeToUser();
  colorTableNode->SetAttribute("Category", "User Generated");
  colorTableNode->HideFromEditorsOff();
  colorTableNode->SaveWithSceneOff();
  colorTableNode->SetSingletonTag(colorTableNode->GetTypeAsString());
  colorTableNode->SetNumberOfColors(6);
  colorTableNode->AddColor("5", 0, 1, 0, 0.2);
  colorTableNode->AddColor("10", 0.5, 1, 0, 0.2);
  colorTableNode->AddColor("15", 1, 1, 0, 0.2);
  colorTableNode->AddColor("20", 1, 0.66, 0, 0.2);
  colorTableNode->AddColor("25", 1, 0.33, 0, 0.2);
  colorTableNode->AddColor("30", 1, 0, 0, 0.2);

  return colorTableNode;
}

//---------------------------------------------------------------------------
int vtkSlicerIsodoseModuleLogic::ComputeIsodose()
{
  // Make sure inputs are initialized
  if (!this->GetMRMLScene() || !this->IsodoseNode)
    {
    return 0;
    }

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->IsodoseNode->GetDoseVolumeNodeId()));

  // Get dose grid scaling and dose units
  const char* doseGridScalingString = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str());
  double doseGridScaling = 1.0;
  if (doseGridScalingString!=NULL)
  {
    doseGridScaling = atof(doseGridScalingString);
  }
  else
  {
    vtkWarningMacro("Dose grid scaling attribute is not set for the selected dose volume. Assuming scaling = 1.");
  }

  const char* doseUnitName = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Hierarchy node for the loaded structure sets
  vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyRootNode = vtkMRMLModelHierarchyNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(
    this->GetIsodoseNode()->GetOutputHierarchyNodeId()));
  // Create root node, if it has not been created yet
  if (modelHierarchyRootNode.GetPointer()==NULL)
  {
    modelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
    modelHierarchyRootNode->AllowMultipleChildrenOn();
    modelHierarchyRootNode->HideFromEditorsOff();
    std::string hierarchyNodeName;
    hierarchyNodeName = std::string(doseVolumeNode->GetName()) + " - isosurface";
    modelHierarchyRootNode->SetName(hierarchyNodeName.c_str());
    this->GetMRMLScene()->AddNode(modelHierarchyRootNode);

    // A hierarchy node needs a display node
    vtkSmartPointer<vtkMRMLModelDisplayNode> modelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    hierarchyNodeName.append("Display");
    modelDisplayNode->SetName(hierarchyNodeName.c_str());
    modelDisplayNode->SetVisibility(1);
    this->GetMRMLScene()->AddNode(modelDisplayNode);
    modelHierarchyRootNode->SetAndObserveDisplayNodeID( modelDisplayNode->GetID() );
  }

  if (modelHierarchyRootNode->GetChildrenNodes().size() > 0)
  {
    modelHierarchyRootNode->RemoveAllChildrenNodes();
  }

  vtkSmartPointer<vtkMRMLColorTableNode> colorNode = vtkMRMLColorTableNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->colorTableID));  
  vtkSmartPointer<vtkLookupTable> lookupTable = colorNode->GetLookupTable();

  vtkSmartPointer<vtkImageChangeInformation> changeInfo = vtkSmartPointer<vtkImageChangeInformation>::New();
  changeInfo->SetInput(doseVolumeNode->GetImageData());
  double origin[3];
  double spacing[3];
  doseVolumeNode->GetOrigin(origin);
  doseVolumeNode->GetSpacing(spacing);
  changeInfo->SetOutputOrigin((origin));
  changeInfo->SetOutputSpacing(-spacing[0], -spacing[1], spacing[2]);
  changeInfo->Update();

  double rgb[3] = {1,1,1};
  double doseLevel = 0.0;
  for (int i = 0; i < colorNode->GetNumberOfColors(); i++)
  {
    double val[6];
    const char* strIsoLevel = colorNode->GetColorName(i);
    double isoLevel = atof(strIsoLevel);
    colorNode->GetColor(i, val);

    vtkSmartPointer<vtkImageMarchingCubes> marchingCubes = vtkSmartPointer<vtkImageMarchingCubes>::New();
    marchingCubes->SetInput(changeInfo->GetOutput());
    marchingCubes->SetNumberOfContours(1); 
    marchingCubes->SetValue(0, isoLevel);
    marchingCubes->Update();

    vtkSmartPointer<vtkPolyData> isoPolyData= marchingCubes->GetOutput();
    if(isoPolyData->GetNumberOfPoints() >= 1)
    {
      vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
      triangleFilter->SetInput(marchingCubes->GetOutput());
      triangleFilter->Update();

      vtkSmartPointer<vtkDecimatePro> decimate = vtkSmartPointer<vtkDecimatePro>::New();
      decimate->SetInput(triangleFilter->GetOutput());
      decimate->SetTargetReduction(0.9);
      decimate->PreserveTopologyOn();
      decimate->Update();

      vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
      normals->SetInput(decimate->GetOutput());
      normals->SetFeatureAngle(45);
      normals->Update();
  
      vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
      displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
      displayNode->SliceIntersectionVisibilityOn();  
      displayNode->VisibilityOn(); 
      displayNode->SetColor(val[0], val[1], val[2]);
      displayNode->SetOpacity(val[3]); // 20120821 set opacity to constant 0.2 per Csaba's request.
    
      // Disable backface culling to make the back side of the contour visible as well
      displayNode->SetBackfaceCulling(0);

      vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
      modelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(modelNode));
      modelNode->SetName(strIsoLevel);
      modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      modelNode->SetAndObservePolyData(normals->GetOutput());
      modelNode->SetHideFromEditors(0);
      modelNode->SetSelectable(1);

      // Add new node to the hierarchy node
      if (modelNode)
      {
        // put the new node in the hierarchy
        vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        this->GetMRMLScene()->AddNode(modelHierarchyNode);
        modelHierarchyNode->SetParentNodeID( modelHierarchyRootNode->GetID() );
        modelHierarchyNode->SetModelNodeID( modelNode->GetID() );
      }
    }
  }

  this->IsodoseNode->SetAndObserveOutputHierarchyNodeId(modelHierarchyRootNode->GetID());
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  return 0;
}
