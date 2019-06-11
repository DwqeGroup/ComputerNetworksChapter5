import java.util.HashSet;
import java.util.Set;

public class Node {  
  
    // 结点名称  
    private String name;
    // 接收到包的数量
    private int right_count = 0;
    private int wrong_count = 0;
    private int send_count = 0;
    
    private Set<Node> relativeNodes = new HashSet<Node>();  
      
    public Node(String name) {  
        this.name = name;  
    }  
      
    public void link(Node... nodes) {  
        for (Node node : nodes) {  
            this.relativeNodes.add(node);  
            node.getRelativeNodes().add(this);  
        }  
    }  
  
    public Set<Node> getRelativeNodes() {  
        return relativeNodes;  
    }  
  
    public void accept(Packet packet) {  
        // 记录当前节点  
        packet.getRoute().add(this.name);  
          
        // 如果计数器仍然等于零 或 当前节点已经是最终节点，则打印路由信息  
        // 否则继续传输，否则输出报文传输路径  
        if (packet.checkdestination(this.name)) {  
            System.out.println("传输成功: " + packet);
            this.right_count += 1;
              
        } else if (packet.getCounter() == 0) {  
            System.out.println("传输失败，已超出生命周期: " + packet); 
            this.wrong_count += 1;
              
        } else {  
            packet.decrement();  
            boolean isAvailableNodeExist = false;  
            for (Node nextNode : relativeNodes) {  
                if (!packet.getRoute().contains(nextNode.getName())) {  
                    isAvailableNodeExist = true;  
                    nextNode.accept(packet.clone());  
                    this.send_count += 1;
                }  
            }  
            if (!isAvailableNodeExist) {  
                System.out.println("传输失败，无法找到下一结点: " + packet);  
                this.wrong_count += 1;
            }  
        }  
    }
  
    public String getName() {  
        return this.name;  
    }
    
    public void show() {
    	System.out.println("结点：" + this.name);
    	System.out.println("丢弃包数量：" + this.wrong_count + " 接受包数量："+ this.right_count + " 转发包数量：" + this.send_count);
    }
      
}  
