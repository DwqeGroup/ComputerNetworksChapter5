import java.util.ArrayList;
import java.util.List;

 
public class Packet implements Cloneable {  
  
    // 计数器  
    private int ttl;  
    
    private String destination;
    // 传输路径  
    private ArrayList<String> route = new ArrayList<String>();  
      
    public Packet(int counter, String dest) {  
        this.ttl = counter;  
        this.destination = dest;
    }  
  
    public int getCounter() {  
        return ttl;  
    }  
  
    public List<String> getRoute() {  
        return route;  
    }  
  
    public void decrement() {  
        this.ttl = this.ttl - 1;  
    }  
    
    public boolean checkdestination(String id) {
    	if(id == this.destination) {
    		return true;
    	} else {
    		return false;
    	}
    }
      
    @SuppressWarnings("unchecked")  
    @Override  
    public Packet clone() {  
        Packet result = null;  
        try {  
            result = (Packet) super.clone();  
            result.route = (ArrayList<String>) this.route.clone();  
        } catch (CloneNotSupportedException e) {  
            e.printStackTrace();  
        }  
        return result;  
    }  
      
    @Override  
    public String toString() {  
        return String.format("报文的传输路径为: %s", route);  
    }  
      
}  
