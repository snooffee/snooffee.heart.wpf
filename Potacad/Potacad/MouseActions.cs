using PotaOCC;
using System;
using System.Drawing;
using System.Threading.Tasks;
using System.Windows.Forms;
using static PotaOCC.MouseHandler;
using static System.Windows.Forms.AxHost;
using Application = System.Windows.Application;
using Control = System.Windows.Forms.Control;
using MouseEventArgs = System.Windows.Forms.MouseEventArgs;

namespace Potacad
{
    public static class MouseActions
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
        public static void SetMouseAction(ViewerHandle viewer, MouseButtons button, bool ctrl, bool shift, MouseState state)
        {
            var dragSetting = GetDragSetting(button, ctrl, shift);
            //zoomdirection = potato.IsChecked;
            zoomdirection = true;
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
        //private static string GetDragSetting(MouseButtons button, bool ctrl, bool shift, EnvSousahouhouModel potato)
        private static string GetDragSetting(MouseButtons button, bool ctrl, bool shift)
        {
            if (button == MouseButtons.Left && !ctrl)
            {
                return "範囲選択";
            }
            else if (button == MouseButtons.Right)
            {
                return "平行移動";
            }
            else if (button == MouseButtons.Left && ctrl)
            {
                return "回転";
            }
            else // Middle
            {
                return "回転";
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
        public static void HandleMouseWheel(ViewerHandle viewer, MouseEventArgs e, MouseState state)
        {
            if (viewer?.NativeHandle == IntPtr.Zero) return;

            bool ctrl = (Control.ModifierKeys & Keys.Control) == Keys.Control;
            bool shift = (Control.ModifierKeys & Keys.Shift) == Keys.Shift;

            zoomdirection = false;

            int dx = e.X - state.LastPos.X;
            int dy = e.Y - state.LastPos.Y;

            string action;
            action = "拡大縮小";

            if (action == "回転")
            {
                Rotate(viewer.ViewPtr, e.X, e.Y);
            }
            else if (action == "平行移動")
            {
                if (dx != 0 || dy != 0)
                {
                    Pan(viewer.ViewPtr, dx, -dy);
                    state.LastPos = e.Location;
                }
            }
            else if (action == "拡大縮小")
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
        //public static async Task Attach(Panel panel, ViewerHandle viewer, EnvSousahouhouModel model)
        public static async Task Attach(Panel panel, ViewerHandle viewer)
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
                SetMouseAction(viewer, e.Button, ctrl, shift, mouseState);

                if (IsLeftButtonClick(e.Button))
                {
                    OnMouseDown(viewer.NativeHandle, e.X, e.Y, IsSelecting(e.Button));

                    // ✅ Start rotation anchor
                    if (mouseState.LeftRotating)
                        StartRotation(viewer.ViewPtr, e.X, e.Y);
                }
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
                HandleMouseWheel(viewer, e, mouseState);
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
                char keyChar = e.KeyCode switch
                {
                    Keys.A => 'A',
                    Keys.B => 'B',
                    Keys.C => 'C',
                    Keys.D => 'D',
                    Keys.E => 'E',
                    Keys.F => 'F',
                    Keys.I => 'I',
                    Keys.L => 'L',
                    Keys.M => 'M',
                    Keys.O => 'O',
                    Keys.P => 'P',
                    Keys.R => 'R',
                    Keys.S => 'S',
                    Keys.T => 'T',
                    Keys.U => 'U',
                    Keys.Q => 'Q',
                    Keys.W => 'W',
                    Keys.Z => 'Z',
                    Keys.Escape => (char)27,  // Escape key
                    _ => '\0' // Default case if no match
                };
                if (e.KeyCode == Keys.A)
                {
                    ResetView(viewer.NativeHandle, 0, 0);
                }
                else
                {
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