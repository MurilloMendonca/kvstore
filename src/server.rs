use std::net::{TcpListener, TcpStream};
use std::thread;
use std::io::Read;
use std::io::Write;
extern "C" {
    fn init_map() -> i32;
    fn get_val(map_id: i32,key: *const u8, key_len: i32, buffer: *mut u8) -> i32;
    fn set_val(map_id: i32, key: *const u8, key_len: i32, value: *const u8, value_len: i32) -> i32;
    fn destroy_map(map_id: i32);
    fn destroy_all_maps();
}

#[derive(Debug)]
enum RequestOrError {
    GetRequest { key: String },
    SetRequest { key: String, value: String },
    DestroyRequest { map_id: Option<i32> },
    HandshakeRequest { map_id: Option<i32> },
    Error(String),
}


// Implement the function to parse the request
// Requests are of the form:
// <g>:<keyLen>:<key>: - get request
// <s>:<keyLen>:<key>:<valueLen>:<value>: - set request
// <h>:<n\map_id>: - handshake request
// <d>:<a\map_id>: - destroy request
fn parse_request(request: &str) -> RequestOrError {
    let parts: Vec<&str> = request.split(':').collect();
    match parts[0] {
        "g" => {
            if let Ok(key_len) = parts[1].parse::<usize>() {
                let key = parts[2].get(0..key_len).unwrap_or_default().to_string();
                RequestOrError::GetRequest { key }
            } else {
                RequestOrError::Error("Invalid key length".to_string())
            }
        }
        "s" => {
            if let (Ok(key_len), Ok(value_len)) = (parts[1].parse::<usize>(), parts[3].parse::<usize>()) {
                let key = parts[2].get(0..key_len).unwrap_or_default().to_string();
                let value = parts[4].get(0..value_len).unwrap_or_default().to_string();
                RequestOrError::SetRequest { key, value }
            } else {
                RequestOrError::Error("Invalid key or value length".to_string())
            }
        }
        "d" => {
            if parts[1].starts_with('a') {
                RequestOrError::DestroyRequest { map_id: None }
            } else if let Ok(map_id) = parts[1].parse::<i32>() {
                RequestOrError::DestroyRequest { map_id: Some(map_id) }
            } else {
                RequestOrError::Error("Invalid map ID".to_string())
            }
        }
        "h" => {
            if parts[1].starts_with('n') {
                RequestOrError::HandshakeRequest { map_id: None }
            } else if let Ok(map_id) = parts[1].parse::<i32>() {
                RequestOrError::HandshakeRequest { map_id: Some(map_id) }
            } else {
                RequestOrError::Error("Invalid map ID".to_string())
            }
        }
        _ => RequestOrError::Error("Invalid request".to_string()),
    }
}

fn process_task(task: RequestOrError, other_map_id: &mut i32) -> Result<String, String> {
    match task {
        RequestOrError::GetRequest { key } => {
            let key_c = Vec::from(key);
            let mut buffer = vec![0; 1 << 20];
            let res = unsafe {
                get_val(*other_map_id as i32, 
                        key_c.as_ptr() as *const u8, 
                        key_c.len() as i32, 
                        buffer.as_mut_ptr() as *mut u8)
            };
            if res != -1 {
                let buff_size = res as usize;
                Ok(String::from_utf8_lossy(&buffer[..buff_size]).to_string())
            } else {
                Err("Failed to get value".to_string())
            }
        }
        RequestOrError::SetRequest { key, value } => {
            let key_c = Vec::from(key.clone());
            let value_c = Vec::from(value.clone());
            let res = unsafe {
                set_val(
                    *other_map_id as i32,
                    key_c.as_ptr() as *const u8,
                    key.len() as i32,
                    value_c.as_ptr() as *const u8,
                    value.len() as i32,
                )
            };
            if res != -1 {
                Ok("OK".to_string())
            } else {
                Err("Error setting value".to_string())
            }
        }
        RequestOrError::DestroyRequest { map_id } => {
            if let Some(map_id) = map_id {
                unsafe {
                    destroy_map(map_id);
                }
            } else {
                unsafe {
                    destroy_all_maps();
                }
            }
            Ok("OK".to_string())
        }
        RequestOrError::HandshakeRequest { map_id } => {
            if let Some(map_id) = map_id {
                *other_map_id = map_id;
                Ok(map_id.to_string())
            } else {
                let res = unsafe { init_map() };
                if res == -1 {
                    Err("Error initializing map".to_string())
                } else {
                    *other_map_id = res;
                    Ok(res.to_string())
                }
            }
        }
        RequestOrError::Error(e) => Err(e),
    }
}

fn handle_client(stream: &mut TcpStream) {
    let mut map_id = -1;
    let mut buffer = [0; 1 << 20];
    loop {
        match stream.read(&mut buffer) {
            Ok(n) => {
                if n == 0 {
                    break;
                }
                let request = String::from_utf8_lossy(&buffer[..n]);
                // println!("Received request: {}", request);
                let task = parse_request(&request);
                // println!("Parsed request: {:?}", task);
                let response = process_task(task, &mut map_id);
                // println!("Response: {:?}", response);
                match response {
                    Ok(response) => {
                        stream.write(response.as_bytes()).unwrap();
                    }
                    Err(e) => {
                        stream.write(e.as_bytes()).unwrap();
                    }
                }
            }
            Err(e) => {
                println!("Error reading from stream: {}", e);
                break;
            }
        }
    }
}

fn main() -> std::io::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    let mut port = "8080";
    if args.len() > 1 {
        port = &args[1];
    }
    let listener = TcpListener::bind("0.0.0.0:".to_owned()+port)?;
    println!("Server listening on port 8080");
    for stream in listener.incoming() {
        match stream {
            Ok(mut stream) => {
                thread::spawn(move || handle_client(&mut stream));
                println!("Client connected");
            }
            Err(e) => {
                println!("Error connecting to client: {}", e);
            }
        }
    }
    Ok(())
}
