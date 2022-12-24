// use std::fmt::Debug;

// #[derive(Debug)]
// struct Image {
//     width: i32,
//     height: i32
// }


// #[derive(Debug)]
// pub enum Message {
//     IncrementHeight,
//     DecrementWidth,
// }

// impl Image {
//     pub fn view(&self) -> () {
//         dbg!("{}", self);
//     }
//     pub fn update(&mut self, message: Message) -> () {
//         match message {
//             Message::IncrementHeight => {
//                 self.height += 1;
//             }
//             Message::DecrementWidth => {
//                 self.width -= 1;
//             }
//         }
//     }
// }

// fn main() {
//     let mut image = Image{width: 1, height: 1};
//     loop {
//         image.update(Message::IncrementHeight);
//         image.update(Message::DecrementWidth);
//         image.view();
//     }

// }

fn main() {
    // stokkr_toolkit::rendering::StkrWindow::new(800, 400);
    stokkr_toolkit::rendering::kit_cairo::draw_rectangle(200, 100);
}
