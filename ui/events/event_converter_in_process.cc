// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/events/event_converter_in_process.h"

#include <sys/mman.h>

#include "base/bind.h"
#include "base/thread_task_runner_handle.h"
#include "ozone/ui/events/event_factory_ozone_wayland.h"
#include "ozone/ui/events/ime_change_observer.h"
#include "ozone/ui/events/output_change_observer.h"
#include "ozone/ui/events/window_change_observer.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"

namespace ui {

EventConverterInProcess::EventConverterInProcess()
    : EventConverterOzoneWayland(),
      weak_ptr_factory_(this) {
  callback_ = base::Bind(&EventConverterInProcess::PostUiEvent,
                         weak_ptr_factory_.GetWeakPtr());
  keyboard_.reset(new KeyboardEvdev(&modifiers_,
      KeyboardLayoutEngineManager::GetKeyboardLayoutEngine(), callback_));
}

EventConverterInProcess::~EventConverterInProcess() {
}

void EventConverterInProcess::MotionNotify(float x, float y) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyMotion,
          weak_ptr_factory_.GetWeakPtr(), x, y));
}

void EventConverterInProcess::ButtonNotify(unsigned handle,
                                           ui::EventType type,
                                           ui::EventFlags flags,
                                           float x,
                                           float y) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyButtonPress,
          weak_ptr_factory_.GetWeakPtr(), handle, type, flags, x, y));
}

void EventConverterInProcess::AxisNotify(float x,
                                         float y,
                                         int xoffset,
                                         int yoffset) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyAxis,
          weak_ptr_factory_.GetWeakPtr(), x, y, xoffset, yoffset));
}

void EventConverterInProcess::PointerEnter(unsigned handle,
                                           float x,
                                           float y) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyPointerEnter,
          weak_ptr_factory_.GetWeakPtr(), handle, x, y));
}

void EventConverterInProcess::PointerLeave(unsigned handle,
                                           float x,
                                           float y) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyPointerLeave,
          weak_ptr_factory_.GetWeakPtr(), handle, x, y));
}

void EventConverterInProcess::KeyNotify(ui::EventType type,
                                        unsigned code) {
  VirtualKeyNotify(type, code);
}

void EventConverterInProcess::VirtualKeyNotify(ui::EventType type,
                                               uint32_t key) {
  keyboard_->OnKeyChange(key, type != ui::ET_KEY_RELEASED);
}

void EventConverterInProcess::TouchNotify(ui::EventType type,
                                          float x,
                                          float y,
                                          int32_t touch_id,
                                          uint32_t time_stamp) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyTouchEvent,
          weak_ptr_factory_.GetWeakPtr(), type, x, y, touch_id, time_stamp));
}

void EventConverterInProcess::CloseWidget(unsigned handle) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyCloseWidget,
          weak_ptr_factory_.GetWeakPtr(), handle));
}

void EventConverterInProcess::OutputSizeChanged(unsigned width,
                                                unsigned height) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyOutputSizeChanged,
          weak_ptr_factory_.GetWeakPtr(), width, height));
}

void EventConverterInProcess::WindowResized(unsigned handle,
                                            unsigned width,
                                            unsigned height) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyWindowResized,
          weak_ptr_factory_.GetWeakPtr(), handle, width, height));
}

void EventConverterInProcess::WindowUnminimized(unsigned handle) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyWindowUnminimized,
          weak_ptr_factory_.GetWeakPtr(), handle));
}

void EventConverterInProcess::WindowDeActivated(unsigned windowhandle) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyWindowDeActivated,
          weak_ptr_factory_.GetWeakPtr(), windowhandle));
}

void EventConverterInProcess::WindowActivated(unsigned windowhandle) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyWindowActivated,
          weak_ptr_factory_.GetWeakPtr(), windowhandle));
}

void EventConverterInProcess::Commit(unsigned handle, const std::string& text) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyCommit,
          weak_ptr_factory_.GetWeakPtr(), handle, text));
}

void EventConverterInProcess::PreeditChanged(unsigned handle,
                                             const std::string& text,
                                             const std::string& commit) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyPreeditChanged,
          weak_ptr_factory_.GetWeakPtr(), handle, text, commit));
}

void EventConverterInProcess::PreeditEnd() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyPreeditEnd,
          weak_ptr_factory_.GetWeakPtr()));
}

void EventConverterInProcess::PreeditStart() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::NotifyPreeditStart,
          weak_ptr_factory_.GetWeakPtr()));
}

void EventConverterInProcess::InitializeXKB(base::SharedMemoryHandle fd,
                                            uint32_t size) {
  char* map_str =
      reinterpret_cast<char*>(mmap(NULL,
                                   size,
                                   PROT_READ,
                                   MAP_SHARED,
                                   fd.fd,
                                   0));
  if (map_str == MAP_FAILED)
    return;

  KeyboardLayoutEngineManager::GetKeyboardLayoutEngine()->
      SetCurrentLayoutByName(map_str);
  munmap(map_str, size);
  close(fd.fd);
}

void EventConverterInProcess::PostUiEvent(scoped_ptr<Event> event) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&EventConverterInProcess::DispatchUiEventTask,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Passed(&event)));
}

void EventConverterInProcess::DispatchUiEventTask(scoped_ptr<Event> event) {
  DispatchEvent(event.get());
}

void EventConverterInProcess::OnDispatcherListChanged() {
}

void EventConverterInProcess::NotifyMotion(float x,
                                           float y) {
  gfx::Point position(x, y);
  ui::MouseEvent mouseev(ui::ET_MOUSE_MOVED,
                         position,
                         position,
                         0,
                         0);
  DispatchEvent(&mouseev);
}

void EventConverterInProcess::NotifyButtonPress(unsigned handle,
                                                ui::EventType type,
                                                ui::EventFlags flags,
                                                float x,
                                                float y) {
  gfx::Point position(x, y);
  ui::MouseEvent mouseev(type,
                         position,
                         position,
                         flags,
                         flags);
  DispatchEvent(&mouseev);

  if (type == ui::ET_MOUSE_RELEASED) {
    ui::WindowChangeObserver* observer =
        ui::EventFactoryOzoneWayland::GetInstance()->
            GetWindowChangeObserver();
    if (observer)
      observer->OnWindowFocused(handle);
  }
}

void EventConverterInProcess::NotifyAxis(float x,
                                         float y,
                                         int xoffset,
                                         int yoffset) {
  gfx::Point position(x, y);
  ui::MouseEvent mouseev(ui::ET_MOUSEWHEEL, position, position, 0, 0);
  ui::MouseWheelEvent wheelev(mouseev, xoffset, yoffset);

  DispatchEvent(&wheelev);
}

void EventConverterInProcess::NotifyPointerEnter(unsigned handle,
                                                 float x,
                                                 float y) {
  ui::WindowChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetWindowChangeObserver();
  if (observer)
    observer->OnWindowEnter(handle);

  gfx::Point position(x, y);
  ui::MouseEvent mouseev(ui::ET_MOUSE_ENTERED, position, position, 0, 0);
  DispatchEvent(&mouseev);
}

void EventConverterInProcess::NotifyPointerLeave(unsigned handle,
                                                 float x,
                                                 float y) {
  ui::WindowChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetWindowChangeObserver();
  if (observer)
    observer->OnWindowLeave(handle);

  gfx::Point position(x, y);
  ui::MouseEvent mouseev(ui::ET_MOUSE_EXITED, position, position, 0, 0);
  DispatchEvent(&mouseev);
}

void EventConverterInProcess::NotifyTouchEvent(ui::EventType type,
                                               float x,
                                               float y,
                                               int32_t touch_id,
                                               uint32_t time_stamp) {
  gfx::Point position(x, y);
  base::TimeDelta time_delta = base::TimeDelta::FromMilliseconds(time_stamp);
  ui::TouchEvent touchev(type, position, touch_id, time_delta);
  DispatchEvent(&touchev);
}

void EventConverterInProcess::NotifyCloseWidget(unsigned handle) {
  ui::WindowChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetWindowChangeObserver();
  if (observer)
    observer->OnWindowClose(handle);
}

void EventConverterInProcess::NotifyOutputSizeChanged(unsigned width,
                                                      unsigned height) {
  ui::OutputChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetOutputChangeObserver();
  if (observer)
    observer->OnOutputSizeChanged(width, height);
}

void EventConverterInProcess::NotifyWindowResized(unsigned handle,
                                                  unsigned width,
                                                  unsigned height) {
  ui::WindowChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetWindowChangeObserver();
  if (observer)
    observer->OnWindowResized(handle, width, height);
}

void EventConverterInProcess::NotifyWindowUnminimized(unsigned handle) {
  ui::WindowChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetWindowChangeObserver();
  if (observer)
    observer->OnWindowUnminimized(handle);
}

void EventConverterInProcess::NotifyWindowDeActivated(unsigned handle) {
  ui::WindowChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetWindowChangeObserver();
  if (observer)
    observer->OnWindowDeActivated(handle);
}

void EventConverterInProcess::NotifyWindowActivated(unsigned handle) {
  ui::WindowChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetWindowChangeObserver();
  if (observer)
    observer->OnWindowActivated(handle);
}

void EventConverterInProcess::NotifyCommit(unsigned handle,
                                           const std::string& text) {
  ui::IMEChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetIMEChangeObserver();
  if (observer)
    observer->OnCommit(handle, text);
}

void EventConverterInProcess::NotifyPreeditChanged(unsigned handle,
                                                   const std::string& text,
                                                   const std::string& commit) {
  ui::IMEChangeObserver* observer =
      ui::EventFactoryOzoneWayland::GetInstance()->GetIMEChangeObserver();
  if (observer)
    observer->OnPreeditChanged(handle, text, commit);
}

void EventConverterInProcess::NotifyPreeditEnd() {
}

void EventConverterInProcess::NotifyPreeditStart() {
}

}  // namespace ui
