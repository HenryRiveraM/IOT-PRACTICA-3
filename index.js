const Alexa = require('ask-sdk-core');
const AWS   = require('aws-sdk');

// ---------- Config AWS IoT y Dynamo ----------
const IotData = new AWS.IotData({
    endpoint: 'a1acybki981kqw-ats.iot.us-east-2.amazonaws.com'
});

const dynamodb   = new AWS.DynamoDB.DocumentClient();
const TABLE_NAME = 'user_thing';

// ================== HELPERS ==================

// Obtiene el thingName desde DynamoDB según el userId de Alexa
async function getThingName(handlerInput) {
    const userId = handlerInput.requestEnvelope.session.user.userId;

    console.log("=== USER ID DE ALEXA (getThingName) ===");
    console.log(userId);
    console.log("=======================================");

    const params = {
        TableName: TABLE_NAME,
        Key: { user_id: userId }
    };

    console.log('Buscando thing para user:', userId);

    try {
        const data = await dynamodb.get(params).promise();
        if (data.Item && data.Item.thing_name) {
            console.log('Thing encontrado:', data.Item.thing_name);
            return data.Item.thing_name;
        } else {
            console.log('No se encontró thing para este usuario, usando por defecto iot_thing');
            return 'iot_thing';          // <- debe coincidir EXACTO con tu thing
        }
    } catch (err) {
        console.log('Error leyendo de DynamoDB:', err);
        return 'iot_thing';
    }
}

// Lee el shadow REAL del thing
async function getShadow(thingName) {
    const params = { thingName };

    console.log('Obteniendo shadow para thing:', thingName);

    try {
        const data       = await IotData.getThingShadow(params).promise();
        const payloadStr = data.payload.toString();
        const payload    = JSON.parse(payloadStr);

        console.log("===== SHADOW COMPLETO =====");
        console.log(JSON.stringify(payload, null, 2));
        console.log("===== SOLO STATE =====");
        console.log(JSON.stringify(payload.state, null, 2));

        if (payload.state) {
            const desired  = payload.state.desired  || {};
            const reported = payload.state.reported || {};

            console.log(">>> ESTADOS DE PUERTAS (DESIRED) <<<");
            console.log("interiorDoor (desired):",  desired.interiorDoor);
            console.log("exteriorDoor (desired):",  desired.exteriorDoor);

            console.log(">>> ESTADOS DE PUERTAS (REPORTED) <<<");
            console.log("interiorDoor (reported):", reported.interiorDoor);
            console.log("exteriorDoor (reported):", reported.exteriorDoor);
        }

        console.log("=====================================");

        return payload.state;
    } catch (err) {
        console.log('Error al obtener el shadow:', err);
        return null;
    }
}

// Cambia el desired.interiorDoor del shadow (OPEN / CLOSE)
async function setInteriorDoorDesired(thingName, newState) {
    const payload = {
        state: {
            desired: {
                interiorDoor: newState
            }
        }
    };

    console.log("Actualizando shadow de", thingName, "a:", newState);

    const params = {
        thingName,
        payload: JSON.stringify(payload)
    };

    try {
        const data = await IotData.updateThingShadow(params).promise();
        console.log("updateThingShadow OK:", data);
    } catch (err) {
        console.log("Error en updateThingShadow:", err);
    }
}

// ================== HANDLERS ==================

// Launch
const LaunchRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'LaunchRequest';
    },
    handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (LaunchRequest) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("========================================");

        const speakOutput =
          'Bienvenido a tu objeto inteligente. ' +
          'Puedes abrir la puerta interior, cerrarla o consultar su estado. ¿Qué deseas hacer?';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput)
            .getResponse();
    }
};

// Abrir puerta interior
const OpenInteriorDoorIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'OpenInteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (OpenInteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("=================================================");

        const thingName = await getThingName(handlerInput);
        await setInteriorDoorDesired(thingName, "OPEN");

        const speakOutput = 'Solicitaste abrir la puerta interior.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres hacer algo más?')
            .getResponse();
    }
};

// Cerrar puerta interior
const CloseInteriorDoorIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'CloseInteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (CloseInteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("==================================================");

        const thingName = await getThingName(handlerInput);
        await setInteriorDoorDesired(thingName, "CLOSE");

        const speakOutput = 'Solicitaste cerrar la puerta interior.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres hacer algo más?')
            .getResponse();
    }
};

// Consultar estado puerta interior
const StateInteriorDoorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'StateInteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (StateInteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("==================================================");

        const thingName   = await getThingName(handlerInput);
        const shadowState = await getShadow(thingName);

        let speakOutput = 'No pude obtener el estado de la puerta interior. Intenta de nuevo más tarde.';

        if (shadowState) {
            const desired  = shadowState.desired  || {};
            const reported = shadowState.reported || {};
            const value = reported.interiorDoor || desired.interiorDoor;

            console.log('Estado interiorDoor desde shadow:', value);

            if (value === 'OPEN') {
                speakOutput = 'La puerta interior está abierta.';
            } else if (value === 'CLOSE' || value === 'CLOSED') {
                speakOutput = 'La puerta interior está cerrada.';
            } else {
                speakOutput = 'La puerta interior tiene un estado desconocido en el dispositivo.';
            }
        }

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres consultar algo más?')
            .getResponse();
    }
};

// Consultar estado puerta exterior
const StateExteriorDoorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'StateExteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (StateExteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("==================================================");

        const thingName   = await getThingName(handlerInput);
        const shadowState = await getShadow(thingName);

        let speakOutput = 'No pude obtener el estado de la puerta exterior. Intenta de nuevo más tarde.';

        if (shadowState) {
            const desired  = shadowState.desired  || {};
            const reported = shadowState.reported || {};
            const value = reported.exteriorDoor || desired.exteriorDoor;

            console.log('Estado exteriorDoor desde shadow:', value);

            if (value === 'OPEN') {
                speakOutput = 'La puerta exterior está abierta.';
            } else if (value === 'CLOSE' || value === 'CLOSED') {
                speakOutput = 'La puerta exterior está cerrada.';
            } else {
                speakOutput = 'La puerta exterior tiene un estado desconocido en el dispositivo.';
            }
        }

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres hacer otra consulta?')
            .getResponse();
    }
};

// ================== HANDLERS GENÉRICOS ==================

const HelpIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.HelpIntent';
    },
    handle(handlerInput) {
        const speakOutput =
          'Puedes decir: abre la puerta interior, cierra la puerta interior, ' +
          'o pregunta por el estado de la puerta interior o exterior. ¿Qué deseas hacer?';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Qué deseas hacer?')
            .getResponse();
    }
};

const CancelAndStopIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && (
                Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.CancelIntent' ||
                Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.StopIntent'
            );
    },
    handle(handlerInput) {
        const speakOutput = 'Hasta pronto.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

const FallbackIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.FallbackIntent';
    },
    handle(handlerInput) {
        const speakOutput =
          'Lo siento, no entendí. Puedes pedir abrir o cerrar la puerta interior, ' +
          'o preguntar por el estado de la puerta interior o exterior.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Qué deseas hacer?')
            .getResponse();
    }
};

const SessionEndedRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'SessionEndedRequest';
    },
    handle(handlerInput) {
        console.log(`~~~~ Session ended: ${JSON.stringify(handlerInput.requestEnvelope)}`);
        return handlerInput.responseBuilder.getResponse();
    }
};

const IntentReflectorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest';
    },
    handle(handlerInput) {
        const intentName  = Alexa.getIntentName(handlerInput.requestEnvelope);
        const speakOutput = `Intentaste ejecutar ${intentName}.`;

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

const ErrorHandler = {
    canHandle() {
        return true;
    },
    handle(handlerInput, error) {
        console.log("=== ERROR EN SKILL ===");
        if (handlerInput &&
            handlerInput.requestEnvelope &&
            handlerInput.requestEnvelope.session &&
            handlerInput.requestEnvelope.session.user) {
            console.log("USER ID (en error):", handlerInput.requestEnvelope.session.user.userId);
        }
        console.log(JSON.stringify(error));
        console.log("===================================");

        const speakOutput = 'Disculpa, hubo un problema. Intenta de nuevo.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Puedes repetir lo que deseas hacer?')
            .getResponse();
    }
};

// ================== EXPORT ==================
exports.handler = Alexa.SkillBuilders.custom()
    .addRequestHandlers(
        LaunchRequestHandler,
        StateExteriorDoorHandler,
        StateInteriorDoorHandler,
        OpenInteriorDoorIntentHandler,
        CloseInteriorDoorIntentHandler,
        HelpIntentHandler,
        CancelAndStopIntentHandler,
        FallbackIntentHandler,
        SessionEndedRequestHandler,
        IntentReflectorHandler
    )
    .addErrorHandlers(ErrorHandler)
    .withCustomUserAgent('sample/hello-world/v1.2')
    .lambda();