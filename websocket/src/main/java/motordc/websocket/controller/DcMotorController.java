package motordc.websocket.controller;
import motordc.websocket.messages.CheckConnectionMessage;
import motordc.websocket.messages.DcMotorChangeRevolutionsMessage;
import motordc.websocket.messages.DcMotorSystemInfo;
import org.springframework.messaging.handler.annotation.DestinationVariable;
import org.springframework.messaging.handler.annotation.MessageMapping;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class DcMotorController {

    private final SimpMessagingTemplate simpMessagingTemplate;

    public DcMotorController(SimpMessagingTemplate simpMessagingTemplate) {
        this.simpMessagingTemplate = simpMessagingTemplate;
    }

    @MessageMapping("/checkconnection/{deviceid}") //To subscribe
    public void checkConnection(@DestinationVariable String deviceid, CheckConnectionMessage message){
        System.out.println(message.toString());
        simpMessagingTemplate.convertAndSend("/connection/status/"+ deviceid, message); //To send
    }
    @MessageMapping("/device-system-info/{to}")
    public void systemInfo(@DestinationVariable String to, DcMotorSystemInfo message){
        System.out.println("Revisando el mensaje " + message.toString() + to);

        simpMessagingTemplate.convertAndSend("/connection/deviceinfo/" + to,message);
    }

    @MessageMapping("/change-rpm/{deviceid}")
    public void changeRpm(@DestinationVariable String deviceid, DcMotorChangeRevolutionsMessage message){
        System.out.println(message.toString() + deviceid);
        simpMessagingTemplate.convertAndSend("/connection/change-rpms/" + deviceid, message);
    }
}
