use std::fs::File;
use cairo::{Context, Format, ImageSurface};

pub fn draw_rectangle(width: i32, height: i32) {
    let surface = ImageSurface::create(Format::ARgb32, width, height).unwrap();
    let cr = Context::new(&surface).unwrap();
    cr.scale(120.0, 120.0);

    cr.set_line_width(0.1);
    cr.set_source_rgb(0.0, 0.0, 0.0);
    cr.rectangle(0.25, 0.25, 0.5, 0.5);
    cr.stroke();

    let mut file = File::create("file.png").unwrap();
    match surface.write_to_png(&mut file) {
        Ok(_) => println!("file.png created"),
        Err(_) => println!("Error create file.png"),
    }
}
