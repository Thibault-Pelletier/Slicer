#ifndef __vtkMouseCursor_h
#define __vtkMouseCursor_h

#include "vtkSlicerSegmentationsModuleLogicExport.h"

#include <vtkObject.h>
#include <variant>
#include <string>

class VTK_SLICER_SEGMENTATIONS_LOGIC_EXPORT vtkMouseCursor : public vtkObject
{
public:
  static vtkMouseCursor* New();
  vtkTypeMacro(vtkMouseCursor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// \brief builtin cursor types
  enum CursorShape
  {
    ArrowCursor,
    UpArrowCursor,
    CrossCursor,
    WaitCursor,
    IBeamCursor,
    SizeVerCursor,
    SizeHorCursor,
    SizeBDiagCursor,
    SizeFDiagCursor,
    SizeAllCursor,
    BlankCursor,
    SplitVCursor,
    SplitHCursor,
    PointingHandCursor,
    ForbiddenCursor,
    WhatsThisCursor,
    BusyCursor,
    OpenHandCursor,
    ClosedHandCursor,
    DragCopyCursor,
    DragMoveCursor,
    DragLinkCursor,
    NoCursor,
  };

  void SetShape(CursorShape shape);

  void SetPath(const std::string& path, int hotX = -1, int hotY = -1);

  int GetCursorShape() const;

  std::string GetCursorPath() const;

  vtkGetMacro(HotX, int);
  vtkSetMacro(HotX, int);

  vtkGetMacro(HotY, int);
  vtkSetMacro(HotY, int);

protected:
  vtkMouseCursor();
  ~vtkMouseCursor() override;

private:
  std::variant<int, std::string> CursorData{ ArrowCursor };
  int HotX{ -1 }, HotY{ -1 };
};

#endif
