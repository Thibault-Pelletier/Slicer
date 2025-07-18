/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __vtkMRMLSequenceBrowserNode_h
#define __vtkMRMLSequenceBrowserNode_h

#include "vtkSlicerSequencesModuleMRMLExport.h"

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkNew.h>

// STD includes
#include <set>
#include <map>

class vtkCollection;
class vtkMRMLSequenceNode;
class vtkIntArray;

class VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT vtkMRMLSequenceBrowserNode : public vtkMRMLNode
{
public:
  static vtkMRMLSequenceBrowserNode* New();
  vtkTypeMacro(vtkMRMLSequenceBrowserNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// ProxyNodeModifiedEvent is invoked when a proxy node is modified
  enum
  {
    ProxyNodeModifiedEvent = 21001,
    IndexDisplayFormatModifiedEvent,
    SequenceNodeModifiedEvent
  };

  /// Modes for determining recording frame rate.
  // Enum is used so that in the future more modes can be added (e.g., fixed frame rate,
  // fixed frame rate matching playback frame rate, etc).
  enum RecordingSamplingModeType
  {
    SamplingAll = 0,
    SamplingLimitedToPlaybackFrameRate,
    NumberOfRecordingSamplingModes // this line must be the last one
  };

  /// Specify what happens when during sequence browsing if a sequence does not contain an item for the
  /// current index.
  enum MissingItemModeType
  {
    MissingItemInvalid = -1,           ///< mode is invalid
    MissingItemCreateFromPrevious = 0, ///< previous item is used for initializing the new item (this is the default mode)
    MissingItemCreateFromDefault,      ///< the new item is created from the default node (typically an empty node)
    MissingItemSetToDefault,           ///< the proxy node is set to the default (empty) node; new item is not created
    MissingItemIgnore,                 ///< the proxy node is not modified
    MissingItemDisplayHidden,          ///< if it is a vtkMRMLDisplayNode the proxy node is hidden, otherwise behaves as MissingItemIgnore
    NumberOfMissingItemModes           // this line must be the last one
  };

  /// Modes displaying index for the user
  enum IndexDisplayModeType
  {
    IndexDisplayAsIndex = 0,
    IndexDisplayAsIndexValue,
    NumberOfIndexDisplayModes // this line must be the last one
  };

  /// Create instance of a GAD node.
  vtkMRMLNode* CreateNodeInstance() override;

  /// Set node attributes from name/value pairs
  void ReadXMLAttributes(const char** atts) override;

  /// Write this node's information to a MRML file in XML format.
  void WriteXML(ostream& of, int indent) override;

  /// Copy the node's attributes to this object
  void CopyContent(vtkMRMLNode* node, bool deepCopy = true) override;

  /// Get unique node XML tag name (like Volume, Model)
  const char* GetNodeTagName() override { return "SequenceBrowser"; };

  /// Set the sequence data node.
  /// Returns the new proxy node postfix.
  std::string SetAndObserveMasterSequenceNodeID(const char* sequenceNodeID);
  /// Get the sequence data node
  vtkMRMLSequenceNode* GetMasterSequenceNode();

  /// Deprecated. Use AddSynchronizedSequenceNodeID instead.
  std::string AddSynchronizedSequenceNode(const char* synchronizedSequenceNodeId);

  /// Adds a node for synchronized browsing. Returns the new proxy node postfix.
  std::string AddSynchronizedSequenceNodeID(const char* synchronizedSequenceNodeId);

  /// Adds a node for synchronized browsing. Returns the new proxy node postfix.
  std::string AddSynchronizedSequenceNode(vtkMRMLSequenceNode* synchronizedSequenceNode);

  /// Removes a node from synchronized browsing
  void RemoveSynchronizedSequenceNode(const char* nodeId);

  /// Remove all sequence nodes (including the master sequence node)
  void RemoveAllSequenceNodes();

  //@{
  /// Returns all synchronized sequence nodes (does not include the master sequence node)
  void GetSynchronizedSequenceNodes(std::vector<vtkMRMLSequenceNode*>& synchronizedDataNodes, bool includeMasterNode = false);
  void GetSynchronizedSequenceNodes(vtkCollection* synchronizedDataNodes, bool includeMasterNode = false);
  //@}

  /// Returns number of synchronized sequence nodes
  int GetNumberOfSynchronizedSequenceNodes(bool includeMasterNode = false);

  /// Deprecated. Use IsSynchronizedSequenceNodeID instead.
  bool IsSynchronizedSequenceNode(const char* sequenceNodeId, bool includeMasterNode = false);

  //@{
  /// Returns true if the node is selected for synchronized browsing
  bool IsSynchronizedSequenceNodeID(const char* sequenceNodeId, bool includeMasterNode = false);
  bool IsSynchronizedSequenceNode(vtkMRMLSequenceNode* sequenceNode, bool includeMasterNode = false);
  //@}

  //@{
  /// Get/Set automatic playback (automatic continuous changing of selected sequence nodes)
  vtkGetMacro(PlaybackActive, bool);
  vtkSetMacro(PlaybackActive, bool);
  vtkBooleanMacro(PlaybackActive, bool);
  //@}

  //@{
  /// Get/Set playback rate in fps (frames per second)
  vtkGetMacro(PlaybackRateFps, double);
  vtkSetMacro(PlaybackRateFps, double);
  //@}

  //@{
  /// Skipping items if necessary to reach requested playback rate. Enabled by default.
  vtkGetMacro(PlaybackItemSkippingEnabled, bool);
  vtkSetMacro(PlaybackItemSkippingEnabled, bool);
  vtkBooleanMacro(PlaybackItemSkippingEnabled, bool);
  //@}

  //@{
  /// Get/Set playback looping (restart from the first sequence node when reached the last one)
  vtkGetMacro(PlaybackLooped, bool);
  vtkSetMacro(PlaybackLooped, bool);
  vtkBooleanMacro(PlaybackLooped, bool);
  //@}

  //@{
  /// Get/Set selected item number. Item number is an integer between 0 and (NumberOfItems - 1).
  vtkGetMacro(SelectedItemNumber, int);
  vtkSetMacro(SelectedItemNumber, int);
  //@}

  /// Set selected item by index value.
  /// If exact match is not required and index is numeric then the best matching data node is returned.
  /// Returns true if the index value is found.
  /// \sa GetItemNumberFromIndexValue
  bool SetSelectedItemByIndexValue(const std::string& indexValue, bool exactMatchRequired = true);

  //@{
  /// Get/set recording of proxy nodes
  vtkGetMacro(RecordingActive, bool);
  void SetRecordingActive(bool recording);
  vtkBooleanMacro(RecordingActive, bool);
  //@}

  //@{
  /// Get/set whether to only record when the master node is modified (or emits an observed event)
  vtkGetMacro(RecordMasterOnly, bool);
  vtkSetMacro(RecordMasterOnly, bool);
  vtkBooleanMacro(RecordMasterOnly, bool);
  //@}

  //@{
  /// Get/set the recording sampling mode
  vtkSetMacro(RecordingSamplingMode, int);
  void SetRecordingSamplingModeFromString(const char* recordingSamplingModeString);
  vtkGetMacro(RecordingSamplingMode, int);
  virtual std::string GetRecordingSamplingModeAsString();
  //@}

  //@{
  /// Helper functions for converting between string and code representation of recording sampling modes
  static std::string GetRecordingSamplingModeAsString(int recordingSamplingMode);
  static int GetRecordingSamplingModeFromString(const std::string& recordingSamplingModeString);
  //@}

  //@{
  /// Helper functions for converting between string and code representation of recording sampling modes
  static std::string GetMissingItemModeAsString(int missingItemMode);
  static MissingItemModeType GetMissingItemModeFromString(const std::string& missingItemModeString);
  //@}

  //@{
  /// Get/set index display mode
  vtkSetMacro(IndexDisplayMode, int);
  void SetIndexDisplayModeFromString(const char* indexDisplayModeString);
  vtkGetMacro(IndexDisplayMode, int);
  virtual std::string GetIndexDisplayModeAsString();
  //@}

  //@{
  /// Get/set format of index value display (used if index type is numeric)
  void SetIndexDisplayFormat(std::string displayFormat);
  vtkGetMacro(IndexDisplayFormat, std::string);
  //@}

  //@{
  /// Helper functions for converting between string and code representation of index display modes
  static std::string GetIndexDisplayModeAsString(int indexDisplayMode);
  static int GetIndexDisplayModeFromString(const std::string& indexDisplayModeString);
  //@}

  //@{
  /// Selects a sequence item for display, returns current selected item number.
  int SelectNextItem(int selectionIncrement = 1);
  int SelectFirstItem();
  int SelectLastItem();
  //@}

  /// Returns number of items in the sequence (number of data nodes in master sequence node)
  int GetNumberOfItems();

  /// Adds proxy nodes from another scene (typically from the main scene). The data node is optionally copied.
  vtkMRMLNode* AddProxyNode(vtkMRMLNode* sourceProxyNode, vtkMRMLSequenceNode* sequenceNode, bool copy = true);

  /// Get proxy corresponding to a sequence node.
  vtkMRMLNode* GetProxyNode(vtkMRMLSequenceNode* sequenceNode);

  /// Deprecated method!
  vtkMRMLNode* GetVirtualOutputDataNode(vtkMRMLSequenceNode* sequenceNode)
  {
    static bool warningLogged = false;
    if (!warningLogged)
    {
      vtkWarningMacro("vtkMRMLSequenceBrowserNode::GetVirtualOutputDataNode is deprecated, use vtkMRMLSequenceBrowserNode::GetProxyNode method instead");
      warningLogged = true;
    }
    return this->GetProxyNode(sequenceNode);
  }

  /// Deprecated method!
  void SetOverwriteTargetNodeName(bool overwrite)
  {
    static bool warningLogged = false;
    if (!warningLogged)
    {
      vtkWarningMacro("vtkMRMLSequenceBrowserNode::SetOverwriteTargetNodeName is deprecated," << " use vtkMRMLSequenceBrowserNode::SetOverwriteProxyName method instead");
      warningLogged = true;
    }
    this->SetOverwriteProxyName(nullptr, overwrite);
  }

  /// Deprecated method!
  void SetDeepCopyVirtualNodes(bool deepcopy)
  {
    static bool warningLogged = false;
    if (!warningLogged)
    {
      vtkWarningMacro("vtkMRMLSequenceBrowserNode::SetDeepCopyVirtualNodes is deprecated, use vtkMRMLSequenceBrowserNode::SetSaveChanges method instead");
      warningLogged = true;
    }
    this->SetSaveChanges(nullptr, !deepcopy);
  }

  /// Get sequence node corresponding to a proxy node.
  vtkMRMLSequenceNode* GetSequenceNode(vtkMRMLNode* proxyNode);

  void GetAllProxyNodes(std::vector<vtkMRMLNode*>& nodes);
  void GetAllProxyNodes(vtkCollection* nodes);

  /// Deprecated method!
  void GetAllVirtualOutputDataNodes(vtkCollection* nodes)
  {
    static bool warningLogged = false;
    if (!warningLogged)
    {
      vtkWarningMacro("vtkMRMLSequenceBrowserNode::GetAllVirtualOutputDataNodes is deprecated," << " use vtkMRMLSequenceBrowserNode::GetAllProxyNodes method instead");
      warningLogged = true;
    }
    this->GetAllProxyNodes(nodes);
  }

  /// Deprecated. Use IsProxyNodeID instead.
  bool IsProxyNode(const char* nodeId);

  /// Returns true if the nodeId belongs to a proxy node managed by this browser node.
  bool IsProxyNodeID(const char* nodeId);

  // TODO: Should these methods be protected? Probably the "world" shouldn't need to know about the postfixes.
  void RemoveProxyNode(const std::string& postfix);

  void RemoveAllProxyNodes();

  /// Returns true if any of the sequence node is allowed to record
  bool IsAnySequenceNodeRecording();

  //@{
  /// Update the proxy node with the content of the sequence.
  bool GetPlayback(vtkMRMLSequenceNode* sequenceNode);
  void SetPlayback(vtkMRMLSequenceNode* sequenceNode, bool playback);
  //@}

  //@{
  /// Add new items to the sequence when sequence recording is activated.
  bool GetRecording(vtkMRMLSequenceNode* sequenceNode);
  void SetRecording(vtkMRMLSequenceNode* sequenceNode, bool recording);
  //@}

  //@{
  /// Enable saving of current proxy node state into the sequence.
  /// If saving is enabled then data is copied from the sequence to into the proxy node using shallow-copy,
  /// which is faster than deep-copy (that is used when save changes disabled).
  /// However, if save changes enabled, proxy node changes are stored in the sequence, therefore users
  /// may accidentally change sequence node content by modifying proxy nodes.
  bool GetSaveChanges(vtkMRMLSequenceNode* sequenceNode);
  void SetSaveChanges(vtkMRMLSequenceNode* sequenceNode, bool save);
  //@}

  //@{
  /// Overwrite proxy node name with name automatically generated from sequence base name and current item index.
  bool GetOverwriteProxyName(vtkMRMLSequenceNode* sequenceNode);
  void SetOverwriteProxyName(vtkMRMLSequenceNode* sequenceNode, bool overwrite);
  //@}

  //@{
  /// Specify what happens when during sequence browsing if a sequence does not contain an item for the
  /// current index.
  /// \sa MissingItemMode
  MissingItemModeType GetMissingItemMode(vtkMRMLSequenceNode* sequenceNode);
  void SetMissingItemMode(vtkMRMLSequenceNode* sequenceNode, MissingItemModeType missingItemMode);
  //@}

  /// Process MRML node events for recording of the proxy nodes
  void ProcessMRMLEvents(vtkObject* caller, unsigned long event, void* callData) override;

  /// Save state of all proxy nodes that recording is enabled for
  virtual void SaveProxyNodesState();

  /// Returns the formatted index value, formatted using the sprintf string provided by IndexDisplayFormat
  /// \sa SetIndexDisplayFormat() GetIndexDisplayFormat()
  std::string GetFormattedIndexValue(int index);

  /// Parses the requestedFormat string to find a validated format for the types contained in typeString.
  /// validatedFormat is set to the first matching sprintf for the input types
  /// prefix and suffix are set to the non-matching components of requestedFormat
  static bool ValidateFormatString(std::string& validatedFormat, std::string& prefix, std::string& suffix, const std::string& requestedFormat, const std::string& typeString);

protected:
  vtkMRMLSequenceBrowserNode();
  ~vtkMRMLSequenceBrowserNode() override;
  vtkMRMLSequenceBrowserNode(const vtkMRMLSequenceBrowserNode&);
  void operator=(const vtkMRMLSequenceBrowserNode&);

  /// Earlier (before November 2015) sequenceNodeRef role name was rootNodeRef.
  /// Change the role name to the new one for compatibility with old data.
  void FixSequenceNodeReferenceRoleName();

  /// Called whenever a new node reference is added
  void OnNodeReferenceAdded(vtkMRMLNodeReference* nodeReference) override;

  /// Called whenever a node reference is removed
  void OnNodeReferenceRemoved(vtkMRMLNodeReference* nodeReference) override;

  std::string GenerateSynchronizationPostfix();
  std::string GetSynchronizationPostfixFromSequence(vtkMRMLSequenceNode* sequenceNode);
  std::string GetSynchronizationPostfixFromSequenceID(const char* sequenceNodeID);

protected:
  bool PlaybackActive{ false };
  double PlaybackRateFps{ 10.0 };
  bool PlaybackItemSkippingEnabled{ true };
  bool PlaybackLooped{ true };
  int SelectedItemNumber{ -1 };

  double RecordingTimeOffsetSec; // difference between universal time and index value
  vtkSetMacro(RecordingTimeOffsetSec, double);
  vtkGetMacro(RecordingTimeOffsetSec, double);

  bool RecordingActive{ false };

  double LastSaveProxyNodesStateTimeSec;
  vtkSetMacro(LastSaveProxyNodesStateTimeSec, double);
  vtkGetMacro(LastSaveProxyNodesStateTimeSec, double);

  bool RecordMasterOnly{ false };
  int RecordingSamplingMode{ vtkMRMLSequenceBrowserNode::SamplingLimitedToPlaybackFrameRate };
  int IndexDisplayMode{ vtkMRMLSequenceBrowserNode::IndexDisplayAsIndexValue };
  std::string IndexDisplayFormat;

  // Unique postfixes for storing references to sequence nodes, proxy nodes, and properties
  // For example, a sequence node reference role name is SEQUENCE_NODE_REFERENCE_ROLE_BASE+synchronizationPostfix
  std::vector<std::string> SynchronizationPostfixes;

  // Counter that is used for generating the unique (only for this class) proxy node postfix strings
  int LastPostfixIndex{ 0 };
  vtkSetMacro(LastPostfixIndex, int);
  vtkGetMacro(LastPostfixIndex, int);

private:
  struct SynchronizationProperties;
  std::map<std::string, SynchronizationProperties*> SynchronizationPropertiesMap;
  SynchronizationProperties* GetSynchronizationPropertiesForSequence(vtkMRMLSequenceNode* sequenceNode);
  SynchronizationProperties* GetSynchronizationPropertiesForPostfix(const std::string& rolePostfix);
};

#endif
