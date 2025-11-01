using PotatoOCC;
using System;
using System.Drawing;
using System.Threading.Tasks;
using System.Windows.Forms;
using static PotatoOCC.MouseHandler;
using static System.Windows.Forms.AxHost;
using Application = System.Windows.Application;
using Control = System.Windows.Forms.Control;
using MouseEventArgs = System.Windows.Forms.MouseEventArgs;

namespace PotatoCAD.Helpers
{
    public static class MouseInteractionHelper
    {
        private static bool zoomdirection = false;
        #region 🖱 Mouse State Class
        public class MouseState
        {
            public bool LeftRotating, LeftPanning, LeftZoom, LeftSelecting;
            public bool RightRotating, RightPanning, RightZoom, RightSelecting;
            public bool WheelRotating, WheelPanning, WheelZoom, WheelSelecting;

            public Point LastPos;
            public bool IsDragging;

            public void Reset()
            {
                LeftRotating = LeftPanning = LeftZoom =
                RightRotating = RightPanning = RightZoom =
                WheelRotating = WheelPanning = WheelZoom =
                LeftSelecting = WheelSelecting = RightSelecting = false;
            }
        }
        #endregion

        #region 🔹 Reset Flags
        public static void ResetMouseFlags(MouseState state) => state.Reset();
        #endregion

        #region 🔹 Set Mouse Action
        public static void SetMouseAction(ViewerHandle viewer, MouseButtons button, bool ctrl, bool shift, EnvSousahouhouModel potato, MouseState state)
        {
            var dragSetting = GetDragSetting(button, ctrl, shift, potato);
            zoomdirection = potato.IsChecked;
            switch (button)
            {
                case MouseButtons.Left:
                    SetActionFlags(dragSetting, ref state.LeftRotating, ref state.LeftPanning, ref state.LeftZoom, viewer, ref state.LeftSelecting);
                    break;

                case MouseButtons.Right:
                    SetActionFlags(dragSetting, ref state.RightRotating, ref state.RightPanning, ref state.RightZoom, viewer, ref state.RightSelecting);
                    break;

                case MouseButtons.Middle:
                    SetActionFlags(dragSetting, ref state.WheelRotating, ref state.WheelPanning, ref state.WheelZoom, viewer, ref state.WheelSelecting);
                    break;
            }
        }
        #endregion

        #region 🔹 Get Drag Setting
        private static string GetDragSetting(MouseButtons button, bool ctrl, bool shift, EnvSousahouhouModel potato)
        {
            if (button == MouseButtons.Left)
            {
                if (ctrl && potato.LeftDrag2Ctrl) return potato.LeftDrag2;
                if (shift && potato.LeftDrag2Shift) return potato.LeftDrag2;
                if (ctrl && potato.LeftDrag3Ctrl) return potato.LeftDrag3;
                if (shift && potato.LeftDrag3Shift) return potato.LeftDrag3;
                return potato.LeftDrag1;
            }
            else if (button == MouseButtons.Right)
            {
                if (ctrl && potato.RightDrag2Ctrl) return potato.RightDrag2;
                if (shift && potato.RightDrag2Shift) return potato.RightDrag2;
                if (ctrl && potato.RightDrag3Ctrl) return potato.RightDrag3;
                if (shift && potato.RightDrag3Shift) return potato.RightDrag3;
                return potato.RightDrag1;
            }
            else // Middle
            {
                if (ctrl && potato.WheelDrag2Ctrl) return potato.WheelDrag2;
                if (shift && potato.WheelDrag2Shift) return potato.WheelDrag2;
                if (ctrl && potato.WheelDrag3Ctrl) return potato.WheelDrag3;
                if (shift && potato.WheelDrag3Shift) return potato.WheelDrag3;
                return potato.WheelDrag1;
            }
        }
        #endregion

        #region 🔹 Set Flags
        private static void SetActionFlags(string dragSetting, ref bool rotating, ref bool panning, ref bool zooming, ViewerHandle viewer, ref bool selecting)
        {
            rotating = panning = zooming = selecting = false;

            switch (dragSetting)
            {
                case "回転":
                case var s when s == (string)Application.Current.Resources["str_kaiten"]:
                    rotating = true;
                    break;

                case "平行移動":
                case var s when s == (string)Application.Current.Resources["str_heikouidou"]:
                    panning = true;
                    break;

                case "拡大縮小":
                case var s when s == (string)Application.Current.Resources["str_kakudaishukushou"]:
                    zooming = true;
                    break;

                case "範囲選択":
                case var s when s == (string)Application.Current.Resources["str_hanisentaku"]:
                    selecting = true;
                    break;
            }
        }
        #endregion

        #region 🔹 Handle Mouse Move
        public static void HandleMouseMove(ViewerHandle viewer, MouseEventArgs e, MouseState state)
        {
            int dx = e.X - state.LastPos.X;
            int dy = e.Y - state.LastPos.Y;

            bool isLeftDown = (Control.MouseButtons & MouseButtons.Left) == MouseButtons.Left;
            bool isRightDown = (Control.MouseButtons & MouseButtons.Right) == MouseButtons.Right;
            bool isMiddleDown = (Control.MouseButtons & MouseButtons.Middle) == MouseButtons.Middle;

            // Rotate
            if ((state.LeftRotating && isLeftDown) ||
                (state.RightRotating && isRightDown) ||
                (state.WheelRotating && isMiddleDown))
                Rotate(viewer.ViewPtr, e.X, e.Y);

            // Pan
            if ((state.LeftPanning && isLeftDown) ||
                (state.RightPanning && isRightDown) ||
                (state.WheelPanning && isMiddleDown))
                Pan(viewer.ViewPtr, dx, -dy);

            // Zoom
            if ((state.LeftZoom && isLeftDown) ||
                (state.RightZoom && isRightDown) ||
                (state.WheelZoom && isMiddleDown))
                Zoom(viewer.ViewPtr, 1.0 + ((zoomdirection ? 1 : -1) * dy * 0.01));

            // ✅ Update last position only if a button is pressed
            if (isLeftDown || isRightDown || isMiddleDown)
                state.LastPos = e.Location;
        }

        #endregion

        #region 🔹 Handle Mouse Wheel
        public static void HandleMouseWheel(ViewerHandle viewer, MouseEventArgs e, MouseState state, EnvSousahouhouModel potato)
        {
            if (viewer?.NativeHandle == IntPtr.Zero) return;

            bool ctrl = (Control.ModifierKeys & Keys.Control) == Keys.Control;
            bool shift = (Control.ModifierKeys & Keys.Shift) == Keys.Shift;

            zoomdirection = potato.IsChecked;

            int dx = e.X - state.LastPos.X;
            int dy = e.Y - state.LastPos.Y;

            string action;
            if (ctrl && potato.WheelRotation2Ctrl)
                action = potato.WheelRotation2;
            else if (shift && potato.WheelRotation2Shift)
                action = potato.WheelRotation2;
            else if (ctrl && potato.WheelRotation3Ctrl)
                action = potato.WheelRotation3;
            else if (shift && potato.WheelRotation3Shift)
                action = potato.WheelRotation3;
            else
                action = potato.WheelRotation1;

            if (action == (string)Application.Current.Resources["str_kaiten"])
            {
                Rotate(viewer.ViewPtr, e.X, e.Y);
            }
            else if (action == (string)Application.Current.Resources["str_heikouidou"])
            {
                if (dx != 0 || dy != 0)
                {
                    Pan(viewer.ViewPtr, dx, -dy);
                    state.LastPos = e.Location;
                }
            }
            else if (action == (string)Application.Current.Resources["str_kakudaishukushou"])
            {
                if (viewer == null || viewer.NativeHandle == IntPtr.Zero || viewer.ViewPtr == IntPtr.Zero)
                    return;

                double factor = e.Delta > 0
                        ? (zoomdirection ? 0.8 : 1.2)
                        : (zoomdirection ? 1.2 : 0.8);
                Zoom(viewer.ViewPtr, factor);


                #region Keep for Review for AIS_TextLabel
                //// Get current OCCT view scale
                //double currentPixelsPerUnit = ShapeDrawer.GetViewZoomRatio(viewer.NativeHandle);


                //// Compute zoom percentage relative to 1.0 scale
                //double zoomPercent = currentPixelsPerUnit * 100.0;

                //// Print it to console
                //Console.WriteLine($"Zoom Factor: {currentPixelsPerUnit:F4}, Zoom Percentage: {zoomPercent:F5}%");


                //// Compute zoom ratio relative to previous
                //double zoomRatio = currentPixelsPerUnit / viewer.PreviousPixelsPerUnit;

                //// Update previous for next wheel event
                //viewer.PreviousPixelsPerUnit = currentPixelsPerUnit;

                //// Rescale all labels using proper zoom ratio
                //ShapeDrawer.RescaleTextHeight(viewer.NativeHandle, zoomRatio* zoomPercent);
                #endregion

            }
        }
        #endregion


        #region 🔹 Attach Mouse Events
        public static async Task Attach(Panel panel, ViewerHandle viewer, EnvSousahouhouModel model)
        {
            // enable face selection once at attach time
            ShapeDrawer.ActivateFaceSelection(viewer.NativeHandle);

            Rectangle rubberBand = Rectangle.Empty;
            bool isDragging = false;

            var mouseState = new MouseState();

            // Helper: Check if selection is active for a given button
            bool IsSelecting(MouseButtons btn) =>
                (btn == MouseButtons.Left && mouseState.LeftSelecting) ||
                (btn == MouseButtons.Right && mouseState.RightSelecting) ||
                (btn == MouseButtons.Middle && mouseState.WheelSelecting);

            bool IsLeftButtonClick(MouseButtons btn) => btn == MouseButtons.Left;
            bool IsRightButtonClick(MouseButtons btn) => btn == MouseButtons.Right;

            panel.BackColor = Color.Transparent;

            panel.MouseDown += (_, e) =>
            {
                mouseState.LastPos = e.Location;
                bool ctrl = (Control.ModifierKeys & Keys.Control) != 0;
                bool shift = (Control.ModifierKeys & Keys.Shift) != 0;

                SetMouseAction(viewer, e.Button, ctrl, shift, model, mouseState);

                if (IsLeftButtonClick(e.Button)) OnMouseDown(viewer.NativeHandle, e.X, e.Y, IsSelecting(e.Button));
            };

            panel.MouseMove += (_, e) =>
            {
                HandleMouseMove(viewer, e, mouseState);
                OnMouseMove(viewer.NativeHandle, e.X, e.Y, panel.Height);

            };
            panel.MouseUp += (_, e) =>
            {
                if (IsLeftButtonClick(e.Button))
                {
                    OnMouseUp(viewer.NativeHandle, e.X, e.Y, panel.Height, panel.Width);
                }

                ResetMouseFlags(mouseState);

            };

            panel.MouseWheel += (_, e) =>
            {
                HandleMouseWheel(viewer, e, mouseState, model);
            };
            panel.MouseEnter += (s, e) => panel.Focus();

            // Panel Paint event to handle custom background and rubber band drawing
            panel.Paint += (_, e) =>
            {
                if (isDragging && !rubberBand.IsEmpty)
                {
                    using (var pen = new Pen(Color.Red, 2)
                    {
                        DashStyle = System.Drawing.Drawing2D.DashStyle.Dash
                    })
                    {
                        e.Graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
                        e.Graphics.DrawRectangle(pen, rubberBand);
                    }
                }
            };

            panel.MouseClick += (s, e) =>
            {

            };
            panel.MouseDoubleClick += (s, e) =>
            {
                if (IsRightButtonClick(e.Button))
                {
                    ShapeDrawer.UpdateSelection(viewer.NativeHandle, e.X, e.Y, true); // update selection under cursor
                    ShapeDrawer.AlignViewToSelectedFace(viewer.NativeHandle);
                }
            };
            panel.Resize += (_, e) =>
            {
                if (viewer?.NativeHandle != IntPtr.Zero)
                {
                    ViewerManager.ResizeViewer(viewer.NativeHandle, panel.Width, panel.Height);
                }
            };

            panel.KeyDown += (_, e) =>
            {
            };

            panel.KeyUp += (_, e) =>
            {
                if (e.KeyCode == Keys.A)
                {
                    ResetView(viewer.NativeHandle, mouseState.LastPos.X, mouseState.LastPos.Y);
                }
                else if (e.KeyCode == Keys.L)
                {
                    char keyChar = 'L';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.B)
                {
                    char keyChar = 'B';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.C)
                {
                    char keyChar = 'C';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.D)
                {
                    char keyChar = 'D';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.E)
                {
                    char keyChar = 'E';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.F)
                {
                    char keyChar = 'F';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.I)
                {
                    char keyChar = 'I';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.M)
                {
                    char keyChar = 'M';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.P)
                {
                    char keyChar = 'P';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.R)
                {
                    char keyChar = 'R';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.S)
                {
                    char keyChar = 'S';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.T)
                {
                    char keyChar = 'T';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.U)
                {
                    char keyChar = 'U';
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
                else if (e.KeyCode == Keys.Escape)  // or e.Key == Key.Escape in WPF
                {
                    char keyChar = (char)27;  // Escape key's ASCII value is 27
                    OnKeyUp(viewer.NativeHandle, (sbyte)keyChar);
                }
            };

        }

        #endregion


        public static void HandleMouseHover(ViewerHandle viewer, MouseEventArgs e, MouseState state)
        {
            if (viewer?.NativeHandle == IntPtr.Zero || viewer.ViewPtr == IntPtr.Zero) return;

            // Get 2D screen coordinates
            int x = e.X;
            int y = e.Y;

            // Check for intersection (ray pick) and highlight the circle
            //HighlightCircleUnderMouse(viewer, x, y);
        }



    }
}