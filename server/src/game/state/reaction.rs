#[derive(Debug)]
pub struct Reaction(bool);

impl Reaction {
    pub fn new() -> Self {
        Reaction(false)
    }

    pub fn new_reacted(reacted: bool) -> Self {
        Reaction(reacted)
    }

    pub fn react_once<F: FnOnce() -> ()>(&mut self, f: F) {
        if !self.0 {
            f();
            self.0 = true;
        }
    }
}
