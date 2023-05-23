package motordc.websocket.messages;

import lombok.*;

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@ToString
public class DcMotorChangeRevolutionsMessage {
    private String from;
    private String to;
    private String action;
    private Integer rpmDesired;
}

