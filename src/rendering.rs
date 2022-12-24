pub mod kit_cairo;
pub mod kit_egl;

use std::{
    // io::{BufWriter, Write, Seek, SeekFrom},
    os::unix::io::AsRawFd,
    process::exit,
    // fs::File
};

use kit_egl::{draw_window, redraw_surface, SurfaceObj};

use wayland_client::{
    event_enum,
    protocol::{wl_compositor, wl_keyboard, wl_pointer, wl_seat, wl_shm::{self, WlShm}, wl_surface, wl_buffer, wl_shm_pool, wl_display::WlDisplay, wl_subsurface::WlSubsurface},
    Display, Filter, GlobalManager, Main, EventQueue, Attached
};
use wayland_protocols::xdg_shell::client::{xdg_surface, xdg_toplevel, xdg_wm_base};

use khronos_egl as egl;

event_enum!(
    Events |
    Pointer => wl_pointer::WlPointer,
    Keyboard => wl_keyboard::WlKeyboard
);


const BUTTON_ID: u32 = 278;
const BUF_X_MAX: i32 = 2000;
const BUF_Y_MAX: i32 = 1200;


pub struct StkrWindow {
    display: Display,
    event_queue: EventQueue,
    attached_display: Attached<WlDisplay>,
    globals: GlobalManager,
    compositor: Main<wl_compositor::WlCompositor>,
    shm: Main<WlShm>
}

impl StkrWindow {
    pub fn new(buf_x: i32, buf_y: i32) -> () {
        let display = Display::connect_to_env().unwrap();
        let mut event_queue = display.create_event_queue();
        let attached_display = (*display).clone().attach(event_queue.token());
        let globals = GlobalManager::new(&attached_display);
        event_queue.sync_roundtrip(&mut (), |_, _, _| unreachable!()).unwrap();
        let compositor = globals.instantiate_exact::<wl_compositor::WlCompositor>(1).unwrap();

        let tmp = tempfile::tempfile().unwrap();

        draw_window(buf_x, buf_y, &tmp, 0);

        let surface = compositor.create_surface();
        let shm = globals.instantiate_exact::<wl_shm::WlShm>(1).unwrap();
        let pool = shm.create_pool(
            tmp.as_raw_fd(),            // RawFd to the tempfile serving as shared memory
            BUF_X_MAX * BUF_Y_MAX * 4, // size in bytes of the shared memory (4 bytes per pixel)
        );
        let buffer = pool.create_buffer(
            0,                        // Start of the buffer in the pool
            buf_x as i32,             // width of the buffer in pixels
            buf_y as i32,             // height of the buffer in pixels
            (buf_x * 4) as i32,       // number of bytes between the beginning of two consecutive lines
            wl_shm::Format::Argb8888, // chosen encoding for the data
        );

        buffer.quick_assign(move |buffer, event, mut data| match event {
            wl_buffer::Event::Release {} => {
                println!("Buffer released!");
                match data.get::<SurfaceObj>() {
                    Some(surface_obj) => {
                        surface_obj.can_change = true;
                        buffer.destroy();
                    },
                    None => println!("No surface initialized")
                }
            }
            _ => unreachable!(),
        });

        let xdg_wm_base = globals.instantiate_exact::<xdg_wm_base::XdgWmBase>(2).expect("Compositor does not support xdg_shell");

        xdg_wm_base.quick_assign(|xdg_wm_base, event, _| {
            if let xdg_wm_base::Event::Ping { serial } = event {
                xdg_wm_base.pong(serial);
            };
        });

        let xdg_surface = xdg_wm_base.get_xdg_surface(&surface);
        xdg_surface.quick_assign(move |xdg_surface, event, _| match event {
            xdg_surface::Event::Configure { serial } => {
                println!("xdg_surface (Configure)");
                xdg_surface.ack_configure(serial);
            }
            _ => unreachable!(),
        });

        let xdg_toplevel = xdg_surface.get_toplevel();
        xdg_toplevel.quick_assign(move |_, event, mut data| match event {
            xdg_toplevel::Event::Close => {
                exit(0);
            }
            xdg_toplevel::Event::Configure { width, height, states } => {
                println!(
                    "xdg_toplevel (Configure) width: {}, height: {}, states: {:?}",
                    width, height, states
                );
                match data.get::<SurfaceObj>() {
                    Some(surface_obj) => {
                        surface_obj.width = width;
                        surface_obj.height = height;
                        if surface_obj.can_change {
                            surface_obj.to_change = true;
                        }
                    },
                    None => println!("No surface initialized")
                }
            }
            _ => unreachable!(),
        });
        xdg_toplevel.set_title("Simple Window".to_string());
        xdg_toplevel.set_app_id("Code".to_string());
        xdg_toplevel.set_max_size(BUF_X_MAX as i32, BUF_Y_MAX as i32);


        let common_filter = Filter::new(move |event, _, mut data| match event {
            Events::Pointer { event, .. } => match event {
                wl_pointer::Event::Enter { surface_x, surface_y, .. } => {
                    println!("Pointer entered at ({}, {}).", surface_x, surface_y);
                }
                wl_pointer::Event::Leave { .. } => {
                    println!("Pointer left.");
                }
                wl_pointer::Event::Motion { surface_x, surface_y, .. } => {
                    println!("Pointer moved to ({}, {}).", surface_x, surface_y);
                }
                wl_pointer::Event::Button { button, state, .. } => {
                    println!("Button {} was {:?}.", button, state);
                    if button == BUTTON_ID && state == wayland_client::protocol::wl_pointer::ButtonState::Released {
                        println!("Change gradient!");
                        match data.get::<SurfaceObj>() {
                            Some(surface_obj) => {
                                if surface_obj.can_change {
                                    surface_obj.grad_offset = (surface_obj.grad_offset + 1) % 3;
                                    surface_obj.to_change = true;
                                }
                            },
                            None => println!("No surface initialized")
                        }
                    }
                }
                _ => {}
            },
            Events::Keyboard { event, .. } => match event {
                wl_keyboard::Event::Enter { .. } => {
                    println!("Gained keyboard focus.");
                }
                wl_keyboard::Event::Leave { .. } => {
                    println!("Lost keyboard focus.");
                }
                wl_keyboard::Event::Key { key, state, .. } => {
                    println!("Key with id {} was {:?}.", key, state);
                }
                _ => (),
            },
        });
        // to be handled properly this should be more dynamic, as more
        // than one seat can exist (and they can be created and destroyed
        // dynamically), however most "traditional" setups have a single
        // seat, so we'll keep it simple here
        let mut pointer_created = false;
        let mut keyboard_created = false;
        globals.instantiate_exact::<wl_seat::WlSeat>(1).unwrap().quick_assign(move |seat, event, _| {
            // The capabilities of a seat are known at runtime and we retrieve
            // them via an events. 3 capabilities exists: pointer, keyboard, and touch
            // we are only interested in pointer & keyboard here
            use wayland_client::protocol::wl_seat::{Capability, Event as SeatEvent};

            if let SeatEvent::Capabilities { capabilities } = event {
                if !pointer_created && capabilities.contains(Capability::Pointer) {
                    // create the pointer only once
                    pointer_created = true;
                    seat.get_pointer().assign(common_filter.clone());
                }
                if !keyboard_created && capabilities.contains(Capability::Keyboard) {
                    // create the keyboard only once
                    keyboard_created = true;
                    seat.get_keyboard().assign(common_filter.clone());
                }
            }
        });

        surface.commit();

        event_queue.sync_roundtrip(&mut (), |_, _, _| { /* we ignore unfiltered messages */ }).unwrap();

        surface.attach(Some(&buffer), 0, 0);
        surface.commit();

        let mut surface_obj = SurfaceObj { surface, to_change: false, grad_offset: 0,
                                                        pool, tmp, width: buf_x as i32, height: buf_y as i32,
                                                        can_change: false};

        loop {
            event_queue.dispatch(&mut surface_obj, |_, _, _| { /* we ignore unfiltered messages */ }).unwrap();
            if surface_obj.to_change {
                surface_obj = redraw_surface(surface_obj);
            }
        }

            let egl = egl::Instance::new(egl::Static);

            let egl_display = egl.get_display(display.get_display_ptr() as *mut std::ffi::c_void).unwrap();
            egl.initialize(egl_display).unwrap();

            let attributes = [
                egl::RED_SIZE, 8,
                egl::GREEN_SIZE, 8,
                egl::BLUE_SIZE, 8,
                egl::NONE
            ];

            let config = egl.choose_first_config(egl_display, &attributes).unwrap().unwrap();

            let context_attributes = [
                egl::CONTEXT_MAJOR_VERSION, 4,
                egl::CONTEXT_MINOR_VERSION, 0,
                egl::CONTEXT_OPENGL_PROFILE_MASK, egl::CONTEXT_OPENGL_CORE_PROFILE_BIT,
                egl::NONE
            ];

            let egl_context = egl.create_context(egl_display, config, None, &context_attributes).unwrap();

        // StkrWindow {display, event_queue, attached_display, globals, compositor, shm}
    }



    pub fn view(&mut self) -> () {

    }
}
