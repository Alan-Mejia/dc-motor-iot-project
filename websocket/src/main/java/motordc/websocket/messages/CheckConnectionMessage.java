package motordc.websocket.messages;

import lombok.*;

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@ToString
public class CheckConnectionMessage {

    private String from;
    private String to;
    private String action;
    private String status;

}
