// Allow single character names so clippy doesn't lint on x, y, r, g, b, which
// are reasonable variable names in this domain.
#![allow(clippy::many_single_char_names)]

use std::{
    // cmp::min,
    io::{BufWriter, Write, Seek, SeekFrom},
    os::unix::io::AsRawFd,
    process::exit,
    time::Instant,
    fs::File
};

use wayland_client::{
    event_enum,
    protocol::{wl_compositor, wl_keyboard, wl_pointer, wl_seat, wl_shm, wl_surface, wl_buffer, wl_shm_pool},
    Display, Filter, GlobalManager, Main
};
use wayland_protocols::xdg_shell::client::{xdg_surface, xdg_toplevel, xdg_wm_base};

// use wayland_egl::WlEglSurface;

// declare an event enum containing the events we want to receive in the iterator
event_enum!(
    Events |
    Pointer => wl_pointer::WlPointer,
    Keyboard => wl_keyboard::WlKeyboard
);

const BUTTON_ID: u32 = 278;
const BUF_X_MAX: u32 = 2000;
const BUF_Y_MAX: u32 = 1200;

struct SurfaceObj {
    surface: Main<wl_surface::WlSurface>,
    to_change: bool,
    can_change: bool,
    grad_offset: u8,
    pool: Main<wl_shm_pool::WlShmPool>,
    tmp: File,
    width: i32,
    height: i32
}

fn draw_window(buf_x: u32, buf_y: u32, mut tmp: &File, grad_offset: u8) {
    // write the contents to it, lets put a nice color gradient
    let now = Instant::now();

    tmp.seek(SeekFrom::Start(0)).unwrap();
    let mut buf = BufWriter::new(&mut tmp);
    for _ in 0..buf_y {
        for x in 0..buf_x {
            let color = 0xFF000000 + (((x * 0xFF) / buf_x) << (8*grad_offset));
            buf.write_all(&color.to_ne_bytes()).unwrap();
        }
    }
    buf.flush().unwrap();

    println!(
        "Time used to create the nice color gradient: {:?}",
        Instant::now().duration_since(now)
    );
}

fn redraw_surface(mut surface_obj: SurfaceObj) -> SurfaceObj {
    draw_window(surface_obj.width as u32, surface_obj.height as u32, &surface_obj.tmp, surface_obj.grad_offset);
    let buffer = surface_obj.pool.create_buffer(
        0,                        // Start of the buffer in the pool
        surface_obj. width,             // width of the buffer in pixels
        surface_obj.height,             // height of the buffer in pixels
        surface_obj.width * 4,       // number of bytes between the beginning of two consecutive lines
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
    surface_obj.surface.attach(Some(&buffer), 0, 0);
    surface_obj.surface.damage(0, 0, surface_obj.width, surface_obj.height);
    surface_obj.surface.commit();

    surface_obj.to_change = false;
    surface_obj
}

fn main() {
    let now = Instant::now();

    let display = Display::connect_to_env().unwrap();

    let mut event_queue = display.create_event_queue();

    let attached_display = (*display).clone().attach(event_queue.token());

    let globals = GlobalManager::new(&attached_display);

    // Make a synchronized roundtrip to the wayland server.
    //
    // When this returns it must be true that the server has already
    // sent us all available globals.
    event_queue.sync_roundtrip(&mut (), |_, _, _| unreachable!()).unwrap();

    /*
     * Create a buffer with window contents
     */

    // buffer (and window) width and height

    // create a tempfile to write the contents of the window on
    let tmp = tempfile::tempfile().expect("Unable to create a tempfile.");

    let buf_x = 800;
    let buf_y = 400;

    draw_window(buf_x, buf_y, &tmp, 0);
    /*
     * Init wayland objects
     */

    // The compositor allows us to creates surfaces
    let compositor = globals.instantiate_exact::<wl_compositor::WlCompositor>(1).unwrap();
    let surface = compositor.create_surface();

    // The SHM allows us to share memory with the server, and create buffers
    // on this shared memory to paint our surfaces
    let shm = globals.instantiate_exact::<wl_shm::WlShm>(1).unwrap();
    let pool = shm.create_pool(
        tmp.as_raw_fd(),            // RawFd to the tempfile serving as shared memory
        (BUF_X_MAX * BUF_Y_MAX * 4) as i32, // size in bytes of the shared memory (4 bytes per pixel)
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

    let xdg_wm_base = globals
        .instantiate_exact::<xdg_wm_base::XdgWmBase>(2)
        .expect("Compositor does not support xdg_shell");

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

    // initialize a seat to retrieve pointer & keyboard events
    //
    // example of using a common filter to handle both pointer & keyboard events
    // let mut maximized = false;
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
                    // if maximized {
                    //     maximized = false;
                    //     xdg_toplevel.unset_maximized();
                    // } else {
                    //     maximized = true;
                    //     xdg_toplevel.set_maximized();
                    // }
                    // xdg_toplevel.show_window_menu();
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

    println!("Time used before enter in the main loop: {:?}", Instant::now().duration_since(now));

    let mut surface_obj = SurfaceObj { surface, to_change: false, grad_offset: 0,
                                                    pool, tmp, width: buf_x as i32, height: buf_y as i32,
                                                    can_change: false};
    loop {
        event_queue.dispatch(&mut surface_obj, |_, _, _| { /* we ignore unfiltered messages */ }).unwrap();
        if surface_obj.to_change {
            surface_obj = redraw_surface(surface_obj);
        }
    }
}
